/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kernel/mutex.h>
#include <lk/debug.h>
#include <dev/i2c.h>
#include <sys/types.h>

#ifndef BMP280_I2C_ADDR
//7-bit address (does not include rd/wr bit)
#define BMP280_I2C_ADDR 0x77
#endif

#ifndef BMP280_DEBUG_LOG
#define BMP280_DEBUG_LOG 0
#endif

#if BMP280_DEBUG_LOG
#define BMP280_TRACE(level, x...) dprintf(level, "BMP280: " x)
#else
#define BMP280_TRACE(level, x...)
#endif

#define BMP280_REG_DIG_T1   (0x88)
#define BMP280_REG_CHIP_ID  (0xd0)
#define BMP280_REG_STATUS   (0xf3)
#define BMP280_REG_CTL_MEAS (0xf4)


typedef struct bmp280_calib_data {
  uint16_t dig_T1;
  int16_t  dig_T2;
  int16_t  dig_T3;
  uint16_t dig_P1;
  int16_t  dig_P2;
  int16_t  dig_P3;
  int16_t  dig_P4;
  int16_t  dig_P5;
  int16_t  dig_P6;
  int16_t  dig_P7;
  int16_t  dig_P8;
  int16_t  dig_P9;
  uint16_t reserved1;
} bmp280_calib_data_t;

typedef struct bmp280_dev {
  int bus;       // i2c bus instance
  int i2c_addr;  // device i2c address (7-bit, no rd/wr bit)
  mutex_t lock;
  bmp280_calib_data_t calib_data;
  volatile bool initialized;
} bmp280_dev_t;

status_t bmp280_init(bmp280_dev_t *dev);

status_t bmp280_set_config(bmp280_dev_t *dev);


static inline void bmp280_dump_calib_data(bmp280_calib_data_t *data) {
  BMP280_TRACE(ALWAYS, "calibration data============\n");
  BMP280_TRACE(ALWAYS, "dig_T1 %u\n", data->dig_T1);
  BMP280_TRACE(ALWAYS, "dig_T2 %d\n", data->dig_T2);
  BMP280_TRACE(ALWAYS, "dig_T3 %d\n", data->dig_T3);
  BMP280_TRACE(ALWAYS, "dig_P1 %u\n", data->dig_P1);
  BMP280_TRACE(ALWAYS, "dig_P2 %d\n", data->dig_P2);
  BMP280_TRACE(ALWAYS, "dig_P3 %d\n", data->dig_P3);
  BMP280_TRACE(ALWAYS, "dig_P4 %d\n", data->dig_P4);
  BMP280_TRACE(ALWAYS, "dig_P5 %d\n", data->dig_P5);
  BMP280_TRACE(ALWAYS, "dig_P6 %d\n", data->dig_P6);
  BMP280_TRACE(ALWAYS, "dig_P7 %d\n", data->dig_P7);
  BMP280_TRACE(ALWAYS, "dig_P8 %d\n", data->dig_P8);
  BMP280_TRACE(ALWAYS, "dig_P9 %d\n", data->dig_P9);
  BMP280_TRACE(ALWAYS, "==================================\n");
}

static inline status_t bmp280_read_regs(bmp280_dev_t *dev, uint8_t reg, uint8_t *buf, size_t len) {
  return i2c_read_reg_bytes(dev->bus, dev->i2c_addr, reg, buf, len);
}

