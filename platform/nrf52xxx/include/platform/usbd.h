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
#ifndef __PLATFORM_NRF_USBD_H
#define __PLATFORM_NRF_USBD_H


#include <stdint.h>
#include <platform/nrf52.h>

#define NRF52_USBD_ENABLE_ENABLED               (1 << 0)

#define NRF52_USBD_USBPULLUP_ENABLED            (1 << 0)


/* EVENTCAUSE register definitions (all are write 1 to clear) */
#define NRF52_USBD_EVENTCAUSE_ISOOUTCRC         (1 << 0)
#define NRF52_USBD_EVENTCAUSE_SUSPEND           (1 << 8)
#define NRF52_USBD_EVENTCAUSE_RESUME            (1 << 9)
#define NRF52_USBD_EVENTCAUSE_USBWUALLOWED      (1 << 10)
#define NRF52_USBD_EVENTCAUSE_READY             (1 << 11)


/* Interrupt enable bits */
#define NRF52_USBD_INT_USBRESET     (1 << 0)
#define NRF52_USBD_INT_STARTED      (1 << 1)
#define NRF52_USBD_INT_ENDEPIN0     (1 << 2)
#define NRF52_USBD_INT_ENDEPIN1     (1 << 3)
#define NRF52_USBD_INT_ENDEPIN2     (1 << 4)
#define NRF52_USBD_INT_ENDEPIN3     (1 << 5)
#define NRF52_USBD_INT_ENDEPIN4     (1 << 6)
#define NRF52_USBD_INT_ENDEPIN5     (1 << 7)
#define NRF52_USBD_INT_ENDEPIN6     (1 << 8)
#define NRF52_USBD_INT_ENDEPIN7     (1 << 9)
#define NRF52_USBD_INT_EP0DATADONE  (1 << 10)
#define NRF52_USBD_INT_ENDISOIN     (1 << 11)
#define NRF52_USBD_INT_ENDEPOUT0    (1 << 12)
#define NRF52_USBD_INT_ENDEPOUT1    (1 << 13)
#define NRF52_USBD_INT_ENDEPOUT2    (1 << 14)
#define NRF52_USBD_INT_ENDEPOUT3    (1 << 15)
#define NRF52_USBD_INT_ENDEPOUT4    (1 << 16)
#define NRF52_USBD_INT_ENDEPOUT5    (1 << 17)
#define NRF52_USBD_INT_ENDEPOUT6    (1 << 18)
#define NRF52_USBD_INT_ENDEPOUT7    (1 << 19)
#define NRF52_USBD_INT_ENDISOOUT    (1 << 20)
#define NRF52_USBD_INT_SOF          (1 << 21)
#define NRF52_USBD_INT_USBEVENT     (1 << 22)
#define NRF52_USBD_INT_EP0SETUP     (1 << 23)
#define NRF52_USBD_INT_EPDATA       (1 << 24)
#define NRF52_USBD_INT_ACCESSFAULT  (1 << 25)

static inline void    nrf52_usbd_pullup_enable(void) {
    NRF_USBD->USBPULLUP = NRF52_USBD_USBPULLUP_ENABLED;
}


static inline bool nrf52_usbd_vbus_ready(void) {
    return ((NRF_POWER->USBREGSTATUS & 0x00000001) != 0);
}

static inline bool nrf52_usbd_output_ready(void) {
    return ((NRF_POWER->USBREGSTATUS & 0x00000002) != 0);
}



static inline uint32_t nrf52_usbd_eventcause_check(uint32_t flag) {
    return (NRF_USBD->EVENTCAUSE & flag);
}

static inline void nrf52_usbd_eventcause_clear(uint32_t flag) {
    NRF_USBD->EVENTCAUSE = flag;
}

void nrf52_usbd_init(void);


#endif

