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
#include <stdio.h>
#include <stdint.h>
#include <arch/arm/cm.h>
#include <dev/gpio.h>
#include <platform/nrf52.h>
#include <platform/nrf_drv_usbd_errata.h>
#include <platform/usbd.h>


#include <target/gpioconfig.h>


#define NRF52_SYSTICK_SRC_CPU  (1 << SysTick_CTRL_CLKSOURCE_Pos)
#define NRF52_SYSTICK_ENABLE   (1 << SysTick_CTRL_ENABLE_Pos)


static uint8_t dma_pending;
static uint32_t simulated_dataepstatus;

static void nrf52_usbd_systick_init(void) {
    SysTick->LOAD = SysTick_LOAD_RELOAD_Msk;
    SysTick->CTRL = NRF52_SYSTICK_SRC_CPU | NRF52_SYSTICK_ENABLE;
}


void nrf52_USBD_IRQ(void)
{
    arm_cm_irq_entry();
    gpio_set(GPIO_LED1,0);
    bool resched = false;

    uint32_t intlist = NRF_USBD->INTENSET;
    uint32_t enabled = intlist;
    uint32_t active = 0;
    volatile uint32_t* evt = &NRF_USBD->EVENTS_USBRESET;
    uint32_t idx=0;
    while(intlist) {
        if ((intlist & 0x00000001) && (evt[idx])){
            //printf("int=%d\n",idx);
            evt[idx] = 0;
            active |= (1 << idx);
        }
        intlist = intlist >> 1;
        idx++;
    }
    if (nrf_drv_usbd_errata_104()) {
        fix_errata_104(dma_pending, enabled, &active, &simulated_dataepstatus);
    }
    if (active & (1 << 23)) {
        printf("Start of frame\n");
        printf("bmRequestType = %0x\n",NRF_USBD->BMREQUESTTYPE);
        printf("bmRequest     = %0x\n",NRF_USBD->BMREQUEST);
        printf("wValue        = %0x%08x\n",NRF_USBD->WVALUEH,NRF_USBD->WVALUEL);
        printf("wIndex        = %0x%08x\n",NRF_USBD->WINDEXH,NRF_USBD->WINDEXL);
        printf("wLength        = %0x%08x\n",NRF_USBD->WLENGTHH,NRF_USBD->WLENGTHL);


    }
    arm_cm_irq_exit(resched);
}



static void nrf52_usbd_start(bool enable_sof)
{

    uint32_t intmask =
       NRF52_USBD_INT_USBRESET     |
       NRF52_USBD_INT_STARTED      |
       NRF52_USBD_INT_ENDEPIN0     |
       NRF52_USBD_INT_EP0DATADONE  |
       NRF52_USBD_INT_ENDEPOUT0    |
       NRF52_USBD_INT_USBEVENT     |
       NRF52_USBD_INT_EP0SETUP     |
       NRF52_USBD_INT_EPDATA       |
       NRF52_USBD_INT_ACCESSFAULT;

    if (enable_sof || nrf_drv_usbd_errata_104())
    {
        intmask |= NRF52_USBD_INT_SOF;
    }

    NRF_USBD->INTENSET = intmask;

    NVIC_EnableIRQ(USBD_IRQn);
    /* Enable pullups */
    nrf52_usbd_pullup_enable();
}





static void nrf52_usbd_enable(void) {
    nrf52_usbd_eventcause_clear(NRF52_USBD_EVENTCAUSE_READY);

    NRF_USBD->ENABLE = NRF52_USBD_ENABLE_ENABLED;

    while (!nrf52_usbd_eventcause_check(NRF52_USBD_EVENTCAUSE_READY)) {
        // Wait for ready flag;
    }

    nrf52_usbd_eventcause_clear(NRF52_USBD_EVENTCAUSE_READY);

    if (nrf_drv_usbd_errata_166())
    {
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x800)) = 0x7E3;
        *((volatile uint32_t *)(NRF_USBD_BASE + 0x804)) = 0x40;
        __ISB();
        __DSB();
    }

    while(!nrf52_usbd_vbus_ready()) {
    }

    printf("usb device enabled\n");

}

void nrf52_usbd_init() {

    printf("USB errata 104 %s\n", (nrf_drv_usbd_errata_104() ? "enabled" : "disabled"));
    printf("USB errata 154 %s\n", (nrf_drv_usbd_errata_154() ? "enabled" : "disabled"));

    if (nrf_drv_usbd_errata_104()) {
        nrf52_usbd_systick_init();
    }

    //Enable USBD
    nrf52_usbd_enable();

    nrf52_usbd_start(true);





}