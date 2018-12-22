/*
 * Copyright (c) 2018 Eric Holland
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdarg.h>
#include <reg.h>
#include <debug.h>
#include <stdio.h>
#include <assert.h>
#include <err.h>
#include <lib/cbuf.h>
#include <arch/arm/cm.h>
#include <arch/ops.h>
#include <dev/gpio.h>
#include <kernel/thread.h>
#include <platform/debug.h>
#include <platform/gpio.h>
#include <platform/spim.h>
#include <target/debugconfig.h>
#include <target/gpioconfig.h>

static spim_cb_t spim0_cb = NULL;

void nrf_spim_init(nrf_spim_dev_t *spim)
{
    spim->instance->ENABLE = 0x07;
    spim->instance->PSEL.SCK  = spim->sclk_pin;
    spim->instance->PSEL.MOSI = spim->mosi_pin;
    spim->instance->PSEL.MISO = spim->miso_pin;
    spim->instance->CONFIG = 0x01;
    gpio_set(spim->sclk_pin, GPIO_OUTPUT);
    gpio_set(spim->mosi_pin, GPIO_OUTPUT);

    spim->instance->FREQUENCY = (uint32_t)spim->speed;
    if (spim->cb) {
        if (spim->instance == NRF_SPIM0) {
            dprintf(SPEW, "setting cb = %p\n", spim->cb);
            spim0_cb = spim->cb;
            NVIC_EnableIRQ(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);
        }
    }
}

static void nrf_spim_irq_handler(NRF_SPIM_Type *dev, spim_cb_t cb){

    if (cb) {

        if (dev->INTENSET & NRF_SPIM_EVENT_ENDTX) {
            dev->EVENTS_ENDTX = 0;
            cb(69);
        }
    }
}

void nrf52_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQ(void)
{
    arm_cm_irq_entry();

    nrf_spim_irq_handler(NRF_SPIM0, spim0_cb);

    arm_cm_irq_exit(true);
}

void nrf_spim_send(nrf_spim_dev_t *spim, uint8_t *buf, uint32_t len) {
    spim->instance->TXD.PTR = (uint32_t)buf;
    spim->instance->TXD.MAXCNT = len;
    if (spim->cb) {
        spim->instance->EVENTS_ENDTX = 0;
        spim->instance->INTENSET = NRF_SPIM_EVENT_ENDTX;
    }
    spim->instance->TASKS_START = 1;
}