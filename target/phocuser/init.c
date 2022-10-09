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
#include <target/gpioconfig.h>

static void target_gpio_init(void) {
    gpio_config(GPIO_LED1, GPIO_OUTPUT);
    gpio_config(GPIO_LED2, GPIO_OUTPUT); 
}

void target_early_init(void) {
    target_gpio_init();
    LED1_ON;
    LED2_ON;

    nrf52_debug_early_init();
}

void target_init(void) {
    nrf52_debug_init();
    dprintf(SPEW,"Target: Phocuser...\n");
}


