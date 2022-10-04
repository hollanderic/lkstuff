/*
 * Copyright (c) 2012 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __TARGET_GPIOCONFIG_H
#define __TARGET_GPIOCONFIG_H

#define GPIO_LED1   7
#define LED1_ON gpio_set(GPIO_LED1,1)
#define LED1_OFF gpio_set(GPIO_LED1,0)

#define GPIO_LED2   (32+9)
#define LED2_ON gpio_set(GPIO_LED2,1)
#define LED2_OFF gpio_set(GPIO_LED2,0)


#define UART0_TX_PIN    22
#define UART0_RX_PIN    32


#endif
