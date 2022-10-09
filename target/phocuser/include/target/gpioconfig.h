/*
 * Copyright (c) 2022 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#ifndef __TARGET_GPIOCONFIG_H
#define __TARGET_GPIOCONFIG_H


#define NRF52_GPIO(BANK, PIN) (32 * BANK + PIN)

#define GPIO_LED1 NRF52_GPIO(0, 7)
#define LED1_ON gpio_set(GPIO_LED1,1)
#define LED1_OFF gpio_set(GPIO_LED1,0)

#define GPIO_LED2  NRF52_GPIO(1, 9)
#define LED2_ON gpio_set(GPIO_LED2,1)
#define LED2_OFF gpio_set(GPIO_LED2,0)

// Debug UART
#define UART0_TX_PIN    NRF52_GPIO(0, 22)
#define UART0_RX_PIN    NRF52_GPIO(1, 0)

// Trinamic SPI bus
#define CS1n NRF52_GPIO(0, 28)
#define SCLK NRF52_GPIO(0, 29)
#define MOSI NRF52_GPIO(0, 30)
#define MISO NRF52_GPIO(0, 31)

// Motor control signals
#define STEP NRF52_GPIO(0, 4)
#define DIR  NRF52_GPIO(0, 5)

// Thermistor
#define THERMO NRF52_GPIO(1, 13)

#endif
