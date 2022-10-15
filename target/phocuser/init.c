/*
 * Copyright (c) 2022 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <target.h>
#include <lk/compiler.h>
#include <dev/gpio.h>
#include <platform/init.h>
#include <platform/spim.h>
#include <target/gpioconfig.h>
#include <app/trinamic.h>


tmc_config_t tmc_pin_configs = {0,1,2,3};

static void target_gpio_init(void) {
    gpio_config(GPIO_LED1, GPIO_OUTPUT);
    gpio_config(GPIO_LED2, GPIO_OUTPUT); 

    // Configure motor driver enable signal and de-assert
    gpio_config(MOTOR_ENn, GPIO_OUTPUT);
    gpio_set(MOTOR_ENn, 1);

    gpio_config(CS1n, GPIO_OUTPUT);
    gpio_set(CS1n, 1);

    gpio_config(TMCCLK, GPIO_OUTPUT);
    gpio_set(TMCCLK, 0);
}

void target_early_init(void) {
    target_gpio_init();
    LED1_ON;
    LED2_ON;

    nrf52_debug_early_init();
}

void target_init(void) {
    nrf52_debug_init();
    nrfx_spim_config_t config;
    spim_init(1, config);
    dprintf(SPEW,"Target: Phocuser...\n");
}


