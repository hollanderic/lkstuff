/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <assert.h>

#include <lk/debug.h>
#include <lk/err.h>

#include <dev/sensor/bmp280.h>

status_t bmp280_init(bmp280_dev_t *dev) {
  ASSERT(dev != NULL);

  mutex_acquire(&dev->lock);
  status_t status;
  uint8_t regval;
  //check chip id
  status = bmp280_read_regs(dev, BMP280_REG_CHIP_ID, &regval, sizeof(regval));

  if ((regval != BMP280_CHIP_ID_VALUE) || (status != NO_ERROR)){
    dev->initialized = false;
    mutex_release(&dev->lock);
    return status ? status : ERR_NOT_FOUND;
  }

  BMP280_TRACE(ALWAYS, "chip_id = 0x%02x\n", regval);
  do {
    status = bmp280_read_regs(dev, BMP280_REG_STATUS, &regval, 1);
    if (status != NO_ERROR) goto init_exit;
  } while ((regval & (BMP280_STATUS_MEASURING | BMP280_STATUS_UPDATING)) != 0);

  status = bmp280_read_regs(dev, BMP280_REG_DIG_T1, (uint8_t*)&dev->calib_data, sizeof(dev->calib_data));

init_exit:
  dev->initialized = (status == NO_ERROR);

  if (dev->initialized) {
    bmp280_dump_calib_data(&dev->calib_data);
  }
  mutex_release(&dev->lock);
  return status;
}

status_t bmp280_set_config(bmp280_dev_t *dev, bmp280_config_t *config) {
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




#if 0
// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
BMP280_S32_t t_fine;
BMP280_S32_t bmp280_compensate_T_int32(BMP280_S32_t adc_T)
{
BMP280_S32_t var1, var2, T;
var1 = ((((adc_T>>3) – ((BMP280_S32_t)dig_T1<<1))) * ((BMP280_S32_t)dig_T2)) >> 11;
var2 = (((((adc_T>>4) – ((BMP280_S32_t)dig_T1)) * ((adc_T>>4) – ((BMP280_S32_t)dig_T1))) >> 12) *
((BMP280_S32_t)dig_T3)) >> 14;
t_fine = var1 + var2;
T = (t_fine * 5 + 128) >> 8;
return T;
}
“”–
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
