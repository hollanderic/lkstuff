/*
 * Copyright 2022 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

// TMC2130 Control Registers
#define TMC2130_GCONF_REG 0x00
#define TMC2130_GSTAT_REG 0x01
#define TMC2130_IOIN_REG  0x04
#define TMC2130_IHOLD_IRUN_REG 0x10
#define TMC2130_TPOWERDOWN_REG 0x11
#define TMC2130_TSTEP_REG 0x12
#define TMC2130_TPWMTHRS_REG 0x13
#define TMC2130_TCOOLTHRS_REG 0x14
#define TMC2130_THIGH_REG 0x15

#define TMC2130_XDIRECT_REG 0x2d

#define TMC2130_VDCMIN_REG 0x33

#define TMC2130_CHOPCONF_REG 0x6c
#define TMC2130_COOLCONF_REG 0x6d
#define TMC2130_DCCTRL_REG 0x6e
#define TMC2130_DRV_STATUS_REG 0x6f
#define TMC2130_PWMCONF_REG 0x70
#define TMC2130_PWM_SCALE_REG 0x71
#define TMC2130_ENCM_CTRL_REG 0x72
#define TMC2130_LOST_STEPS_REG 0x73



typedef struct tmc_config {
	uint32_t CSnPIN;
	uint32_t MISOPIN;
	uint32_t MOSIPIN;
	uint32_t SCLKPIN;
} tmc_config_t;

uint32_t trinamic_drv_init(void);

uint32_t trinamic_spi_write(uint8_t address, uint32_t outval, uint32_t* inval);
uint32_t trinamic_spi_read(uint8_t address, uint32_t* inval);


