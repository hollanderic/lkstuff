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

#ifndef __PLATFORM_NRF52_SPIM_H
#define __PLATFORM_NRF52_SPIM_H
#include <platform/nrf52xxx.h>

#define NRF_SPIM_EVENT_STOPPED 0x00000002
#define NRF_SPIM_EVENT_ENDRX 0x00000010
#define NRF_SPIM_EVENT_END 0x00000040
#define NRF_SPIM_EVENT_ENDTX 0x00000100
#define NRF_SPIM_EVENT_STARTED (1 << 19)



typedef int (*spim_cb_t) (int arg);

typedef enum {
    SPIM_SPEED_125K = 0x02000000,
    SPIM_SPEED_250K = 0x04000000,
    SPIM_SPEED_500K = 0x08000000,
    SPIM_SPEED_1M = 0x10000000,
    SPIM_SPEED_2M = 0x20000000,
    SPIM_SPEED_4M = 0x40000000,
    SPIM_SPEED_8M = 0x80000000,
} spim_speed_t;

typedef struct {
    NRF_SPIM_Type *instance;
    uint8_t    sclk_pin;
    uint8_t    mosi_pin;
    uint8_t    miso_pin;
    spim_speed_t   speed;
    spim_cb_t  cb;
} nrf_spim_dev_t;

void nrf_spim_init(nrf_spim_dev_t *spim);
void nrf_spim_send(nrf_spim_dev_t *spim, uint8_t *buf, uint32_t len);

#endif
