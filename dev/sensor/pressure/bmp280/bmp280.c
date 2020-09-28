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
    if (dev == NULL) {
        return ERR_INVALID_ARGS;
    }
    uint8_t config_reg, ctl_reg;
    {
        uint8_t temp[2];
        bmp280_read_regs(dev, BMP280_REG_CTL_MEAS, temp, sizeof(temp));
    }
    config_reg = (config->tstby << BMP280_CONFIG_T_SB_POS) |
                 (config->filt << BMP280_CONFIG_FILTER_POS);

    ctl_reg = (config->mode << BMP280_CTL_MEAS_MODE_POS) |
              (config->posr << BMP280_CTL_MEAS_OSRS_P_POS) |
              (config->tosr << BMP280_CTL_MEAS_OSRS_T_POS);

    status_t status = bmp280_write_reg(dev, BMP280_REG_CONFIG, config_reg);
    if (status != NO_ERROR) {
        return status;
    }
    status = bmp280_write_reg(dev, BMP280_REG_CTL_MEAS, ctl_reg);

    uint8_t temp[2];
    bmp280_read_regs(dev, BMP280_REG_CTL_MEAS, temp, 2);
    return status;
}

bmp280_dev_t *bmp280_get_dev_at_addr(int bus, int i2c_addr) {
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
bmp280_dev_t *bmp280_init(int bus, int i2c_addr, bmp280_config_t *config) {

    bmp280_dev_t *dev = bmp280_get_dev_at_addr(bus, i2c_addr);
    if (dev != NULL) {
        BMP280_TRACE(CRITICAL, "bmp280 device bus:%d address:0x%02x already exits\n",
                     bus, i2c_addr);
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

    if ((regval != BMP280_CHIP_ID_VALUE) || (status != NO_ERROR)) {
        status = status ? status : ERR_NOT_FOUND;
        goto init_exit;
    }

    BMP280_TRACE(ALWAYS, "chip_id = 0x%02x\n", regval);
    do {
        status = bmp280_read_regs(dev, BMP280_REG_STATUS, &regval, 1);
        if (status != NO_ERROR) goto init_exit;
    } while ((regval & (BMP280_STATUS_MEASURING | BMP280_STATUS_UPDATING)) != 0);

    status = bmp280_read_regs(dev, BMP280_REG_DIG_T1, (uint8_t *)&dev->calib_data,
                              sizeof(dev->calib_data));
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

bmp280_dev_t *bmp280_get_dev_at_index(int idx) {
    if ((size_t)idx >= list_length(&dev_list)) {
        return NULL;
    }
    bmp280_dev_t *devs = containerof(dev_list.next, bmp280_dev_t, node);
    return &devs[idx];
}

status_t bmp280_set_config(bmp280_dev_t *dev, bmp280_config_t *config) {
    mutex_acquire(&dev->lock);
    status_t status = bmp280_set_config_locked(dev, config);
    mutex_release(&dev->lock);
    return status;
}

// Returns temperature in DegC
// Algorithm found in BMP280 datasheet.
static double bmp280_compensate_T(bmp280_calib_data_t *calib, int32_t adc_T, double *t_fine) {
    DEBUG_ASSERT(calib != NULL);
    DEBUG_ASSERT(t_fine != NULL);

    double var1, var2, T;
    var1 = (((double)adc_T/16384.0) - ((double)calib->dig_T1)/1024.0) * ((double)calib->dig_T2);
    var2 = ((((double)adc_T)/131072.0 - ((double)calib->dig_T1)/8192.0) *
            (((double)adc_T)/131072.0 - ((double)calib->dig_T1)/8192.0)) * ((double)calib->dig_T3);
    *t_fine = var1 + var2;
    T = (var1 + var2)/5120.0;
    return T;
}

static double bmp280_compensate_P(bmp280_calib_data_t *calib, int32_t adc_P, double t_fine) {
    DEBUG_ASSERT(calib != NULL);

    double var1 = t_fine/2.0 - 64000.0;
    double var2 = var1 * var1 * ((double)calib->dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)calib->dig_P5) * 2.0;
    var2 = (var2 / 4.0) + (((double)calib->dig_P4) * 65536.0);
    var1 = (((double)calib->dig_P3) * var1 * var1 / 524288.0 +
            ((double)calib->dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0) * ((double)calib->dig_P1);
    double Pa = 1048576.0 - (double)adc_P;
    Pa = (Pa - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)calib->dig_P9) * Pa * Pa / 2147483648.0;
    var2 = Pa * ((double)calib->dig_P8) /  32768.0;
    Pa = Pa + (var1 + var2 + ((double)calib->dig_P7)) / 16.0;
    return Pa;
}

static status_t bmp280_get_readings_raw(bmp280_dev_t *dev, int32_t *temp, int32_t *press) {
    if ((dev == NULL) || (temp == NULL) || (press == NULL)) {
        return ERR_INVALID_ARGS;
    }
    uint8_t data[6];
    mutex_acquire(&dev->lock);
    status_t status = bmp280_read_regs(dev, BMP280_REG_PRESS_MSB, data, sizeof(data));
    mutex_release(&dev->lock);
    if (status == NO_ERROR) {
        *press = ((data[0] << 16) + (data[1] << 8) + data[2]) >> 4;
        *temp = ((data[3] << 16) + (data[4] << 8) + data[5]) >> 4;
    } else {
        BMP280_TRACE(ALWAYS, "Error in read temperature: %d\n",status);
    }
    return status;
}

status_t bmp280_get_readings(bmp280_dev_t *dev, double *temp, double *press) {
  int32_t temp_raw, press_raw;
  double t_fine;
  status_t status = bmp280_get_readings_raw(dev, &temp_raw, &press_raw);
  if (status != NO_ERROR) { return status; }
  *temp = bmp280_compensate_T(&dev->calib_data, temp_raw, &t_fine);
  *press = bmp280_compensate_P(&dev->calib_data, press_raw, t_fine);
  return status;
}


void read_baro(void);

STATIC_COMMAND_START
STATIC_COMMAND("read_baro", "read barometer (and temperature)", (console_cmd_func)&read_baro)
STATIC_COMMAND_END(bmp280);

void read_baro(void) {
    bmp280_dev_t *dev;
    dev = bmp280_get_dev_at_index(0);
    double  Tc, Pa;

    status_t status = bmp280_get_readings(dev, &Tc, &Pa);
    if (!status) {
        double Tf = (Tc) * 9.0/5.0 + 32.0;
        double inHg = Pa * 0.00029599831;
        Pa = Pa / 100.0;
        dprintf(ALWAYS, "Temperature = %f C(%f F) Pressure = %f hPa (%f mmHg)\n", Tc, Tf, Pa, inHg);
    } else {
        BMP280_TRACE(ALWAYS, "Error in read temperature: %d\n",status);
    }
}

APP_START(bmp280)
.flags = 0,
APP_END
