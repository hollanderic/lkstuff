/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <kernel/mutex.h>
#include <lk/debug.h>
#include <lk/list.h>
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

#define BMP280_REG_DIG_T1     (0x88)
#define BMP280_REG_CHIP_ID    (0xd0)
#define BMP280_REG_STATUS     (0xf3)
#define BMP280_REG_CTL_MEAS   (0xf4)
#define BMP280_REG_CONFIG     (0xf5)
#define BMP280_REG_PRESS_MSB  (0xf7)
#define BMP280_REG_PRESS_LSB  (0xf8)
#define BMP280_REG_PRESS_XLSB (0xf9)
#define BMP280_REG_TEMP_MSB   (0xfa)
#define BMP280_REG_TEMP_LSB   (0xfb)
#define BMP280_REG_TEMP_XLSB  (0xfc)

#define BMP280_CTL_MEAS_MODE_POS  (0)
#define BMP280_CTL_MEAS_MODE_MASK (0x03 << BMP280_CTL_MEAS_MODE_POS)
#define BMP280_CTL_MEAS_OSRS_P_POS  (2)
#define BMP280_CTL_MEAS_OSRS_P_MASK (0x07 << BMP280_CTL_MEAS_OSRS_P_POS)
#define BMP280_CTL_MEAS_OSRS_T_POS  (5)
#define BMP280_CTL_MEAS_OSRS_T_MASK (0x07 << BMP280_CTL_MEAS_OSRS_T_POS)

#define BMP280_CONFIG_FILTER_POS (2)
#define BMP280_CONFIG_FILTER_MASK (0x07 << BMP280_CONFIG_FILTER_POS)
#define BMP280_CONFIG_T_SB_POS (5)
#define BMP280_CONFIG_T_SB_MASK (0x07 << BMP280_CONFIG_T_SB_POS)

#define BMP280_STATUS_MEASURING  (1 << 3)
#define BMP280_STATUS_UPDATING   (1 << 0)

#define BMP280_CHIP_ID_VALUE  (0x58)

typedef enum {
  BMP280_POSR_SKIP,
  BMP280_POSR_X1,
  BMP280_POSR_X2,
  BMP280_POSR_X4,
  BMP280_POSR_X8,
  BMP280_POSR_X16,
} bmp280_pressure_osr_t;

typedef enum {
  BMP280_TOSR_SKIP,
  BMP280_TOSR_X1,
  BMP280_TOSR_X2,
  BMP280_TOSR_X4,
  BMP280_TOSR_X8,
  BMP280_TOSR_X16,
} bmp280_temperature_osr_t;

typedef enum {
  BMP280_MODE_SLEEP = 0,
  BMP280_MODE_FORCED = 1,
  BMP280_MODE_NORMAL = 3
} bmp280_mode_t;

typedef enum {
  BMP280_TSTBY_MS_0P5 = 0,
  BMP280_TSTBY_MS_62P5 = 1,
  BMP280_TSTBY_MS_125 = 2,
  BMP280_TSTBY_MS_250 = 3,
  BMP280_TSTBY_MS_500 = 4,
  BMP280_TSTBY_MS_1000 = 5,
  BMP280_TSTBY_MS_2000 = 6,
  BMP280_TSTBY_MS_4000 = 7
} bmp280_tstby_t;

typedef enum {
  BMP280_FILT_OFF = 0,
  BMP280_FILT_2 = 1,
  BMP280_FILT_4 = 2,
  BMP280_FILT_8 = 3,
  BMP280_FILT_16 = 4
} bmp280_filt_t;

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
  struct list_node node;
  int bus;       // i2c bus instance
  int i2c_addr;  // device i2c address (7-bit, no rd/wr bit)
  mutex_t lock;
  bmp280_calib_data_t calib_data;
  int idx;
} bmp280_dev_t;

typedef struct bmp280_config {
  bmp280_pressure_osr_t posr;
  bmp280_temperature_osr_t tosr;
  bmp280_tstby_t tstby;
  bmp280_mode_t mode;
  bmp280_filt_t filt;
} bmp280_config_t;

bmp280_dev_t * bmp280_init(int bus, int i2c_addr, bmp280_config_t *config);
bmp280_dev_t * bmp280_find_dev(int bus, int i2c_addr);
bmp280_dev_t * bmp280_get_dev_at_index(int idx);
status_t bmp280_read_temperature(bmp280_dev_t *dev, int32_t *out);

status_t bmp280_set_config(bmp280_dev_t *dev, bmp280_config_t *config);
status_t bmp280_get_config(bmp280_dev_t *dev, bmp280_config_t *config);

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
static inline status_t bmp280_write_regs(bmp280_dev_t *dev, uint8_t reg, uint8_t *buf, size_t len) {
  return i2c_write_reg_bytes(dev->bus, dev->i2c_addr, reg, buf, len);
}

