/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <app.h>
#include <assert.h>
#include <malloc.h>

#include <lk/console_cmd.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/list.h>

#include <dev/sensor/bmp280.h>

struct list_node dev_list = LIST_INITIAL_VALUE(dev_list);

static status_t bmp280_set_config_locked(bmp280_dev_t *dev, bmp280_config_t *config) {
  uint8_t config_reg, ctl_reg;

  config_reg = (config->tstby << BMP280_CONFIG_T_SB_POS) |
               (config->filt << BMP280_CONFIG_FILTER_POS);

  ctl_reg = (config->mode << BMP280_CTL_MEAS_MODE_POS) |
            (config->posr << BMP280_CTL_MEAS_OSRS_P_POS) |
            (config->tosr << BMP280_CTL_MEAS_OSRS_T_POS);

  status_t status = bmp280_write_regs(dev, BMP280_REG_CONFIG, &config_reg, sizeof(config_reg));
  if (status != NO_ERROR) {
    return status;
  }
  return bmp280_write_regs(dev, BMP280_REG_CTL_MEAS, &ctl_reg, sizeof(ctl_reg));
}

bmp280_dev_t * bmp280_find_dev(int bus, int i2c_addr) {
  if (list_is_empty(&dev_list)) {
    return NULL;
  }
  bmp280_dev_t *dev_ptr;
  list_for_every_entry(&dev_list, dev_ptr, bmp280_dev_t, node) {
    if ((dev_ptr->bus == bus) && (dev_ptr->i2c_addr == i2c_addr)) {
      return dev_ptr;
    }
  }
  return NULL;
}

/*
  Checks to see if bus:addr device is already initialized.  If so return error.
  -Checks for proper chip id
  -Reads calibration data
  -If config structure is provided, will set config of device
*/
bmp280_dev_t * bmp280_init(int bus, int i2c_addr, bmp280_config_t *config) {

  bmp280_dev_t *dev = bmp280_find_dev(bus, i2c_addr);
  if (dev != NULL) {
    BMP280_TRACE(CRITICAL, "bmp280 device bus:%d address:0x%02x already exits\n", bus, i2c_addr);
    return NULL;
  }

  dev = malloc(sizeof(bmp280_dev_t));
  if (dev == NULL) {
    return NULL;
  }

  dev->bus = bus;
  dev->i2c_addr = i2c_addr;
  mutex_init(&dev->lock);

  //Since this is a new device we won't use the lock.
  status_t status;
  uint8_t regval;
  //check chip id
  status = bmp280_read_regs(dev, BMP280_REG_CHIP_ID, &regval, sizeof(regval));

  if ((regval != BMP280_CHIP_ID_VALUE) || (status != NO_ERROR)){
    status = status ? status : ERR_NOT_FOUND;
    goto init_exit;
  }

  BMP280_TRACE(ALWAYS, "chip_id = 0x%02x\n", regval);
  do {
    status = bmp280_read_regs(dev, BMP280_REG_STATUS, &regval, 1);
    if (status != NO_ERROR) goto init_exit;
  } while ((regval & (BMP280_STATUS_MEASURING | BMP280_STATUS_UPDATING)) != 0);

  status = bmp280_read_regs(dev, BMP280_REG_DIG_T1, (uint8_t*)&dev->calib_data, sizeof(dev->calib_data));
  if (status != NO_ERROR) {
    goto init_exit;
  }
  bmp280_dump_calib_data(&dev->calib_data);
  if (config != NULL) {
    status = bmp280_set_config(dev, config);
  }
  if (status == NO_ERROR) {
    dev->idx = list_length(&dev_list);
    list_add_tail(&dev_list, &dev->node);
  }

init_exit:
  if (status != NO_ERROR) {
    free(dev);
    return NULL;
  }
  return dev;
}

bmp280_dev_t * bmp280_get_dev_at_index(int idx) {
  if ((size_t)idx >= list_length(&dev_list)) {
    return NULL;
  }
  bmp280_dev_t * devs = containerof(dev_list.next, bmp280_dev_t, node);
  return &devs[idx];
}

status_t bmp280_set_config(bmp280_dev_t *dev, bmp280_config_t *config) {
  mutex_acquire(&dev->lock);
  status_t status = bmp280_set_config_locked(dev, config);
  mutex_release(&dev->lock);
  return status;
}

status_t bmp280_read_temperature(bmp280_dev_t *dev, int32_t *out) {
  uint8_t data[3];
  status_t status = bmp280_read_regs(dev, BMP280_REG_TEMP_MSB, data, sizeof(data));
  if (status == NO_ERROR) {
    *out = (data[0] << 16) + (data[1] << 8) + data[2];
  }
  return status;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
int32_t t_fine;
int32_t bmp280_compensate_T_int32(bmp280_calib_data_t *calib, int32_t adc_T)
{
  int32_t var1, var2, T;
  var1 = ((((adc_T>>3) - ((int32_t)calib->dig_T1<<1))) * ((int32_t)calib->dig_T2)) >> 11;
  var2 = (((((adc_T>>4) - ((int32_t)calib->dig_T1)) * ((adc_T>>4) - ((int32_t)calib->dig_T1))) >> 12) *
  ((int32_t)calib->dig_T3)) >> 14;
  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
return T;
}




void read_temp(void);

STATIC_COMMAND_START
STATIC_COMMAND("read_temp", "read temperature", (console_cmd_func)&read_temp)
STATIC_COMMAND_END(bmp280);

void read_temp(void) {
    bmp280_dev_t * dev;
    dev = bmp280_get_dev_at_index(0);
    int32_t T, Tc;

    bmp280_read_temperature(dev, &T);
    Tc = bmp280_compensate_T_int32(&dev->calib_data, T);

    dprintf(ALWAYS, "Raw: %d   Compensated: %d  Tfine: %d\n", T, Tc, t_fine);
}

APP_START(bmp280)
.flags = 0,
APP_END



#if 0
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BMP280_U32_t bmp280_compensate_P_int64(BMP280_S32_t adc_P)
{
BMP280_S64_t var1, var2, p;
var1 = ((BMP280_S64_t)t_fine) – 128000;
var2 = var1 * var1 * (BMP280_S64_t)dig_P6;
var2 = var2 + ((var1*(BMP280_S64_t)dig_P5)<<17);
var2 = var2 + (((BMP280_S64_t)dig_P4)<<35);
var1 = ((var1 * var1 * (BMP280_S64_t)dig_P3)>>8) + ((var1 * (BMP280_S64_t)dig_P2)<<12);
var1 = (((((BMP280_S64_t)1)<<47)+var1))*((BMP280_S64_t)dig_P1)>>33;
if (var1 == 0)
{
return 0; // avoid exception caused by division by zero
}
p = 1048576-adc_P;
p = (((p<<31)-var2)*3125)/var1;
var1 = (((BMP280_S64_t)dig_P9) * (p>>13) * (p>>13)) >> 25;
var2 = (((BMP280_S64_t)dig_P8) * p) >> 19;
p = ((p + var1 + var2) >> 8) + (((BMP280_S64_t)dig_P7)<<4);
return (BMP280_U32_t)p;
#endif
