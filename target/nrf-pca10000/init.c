/*
 * Copyright (c) 2012 Travis Geiselbrecht
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
#include <err.h>
#include <debug.h>
#include <target.h>
#include <compiler.h>
#include <dev/gpio.h>
#include <kernel/timer.h>
#include <lib/ble.h>
#include <platform/gpio.h>
#include <platform/nrf51.h>
#include <target/gpioconfig.h>
#include <dev/ble_radio.h>


static bool heartbeat = false;
static ble_t ble1;
const char loadstr[] = "lk Beacon";

static thread_t *blinkthread;


void target_early_init(void)
{
    NRF_CLOCK->XTALFREQ = CLOCK_XTALFREQ_XTALFREQ_16MHz;

	/* configure the usart1 pins */
	gpio_config(GPIO_LED1, GPIO_OUTPUT);
    gpio_config(GPIO_LED2, GPIO_OUTPUT);
    gpio_config(GPIO_LED3, GPIO_OUTPUT);

	gpio_set(GPIO_LED1,1);
	gpio_set(GPIO_LED2,1);
	gpio_set(GPIO_LED3,1);

    gpio_config(UART0_RTS_PIN, GPIO_OUTPUT);
    gpio_set(UART0_RTS_PIN,0);              //placate flow control requirements of pca10000

	nrf51_debug_early_init();
}

static int blinker(void * args){
    while (1) {
        //ble_pdu_add_shortname(&ble1,loadstr,9);
        if (heartbeat) {
            heartbeat = false;
            gpio_set(GPIO_LED1,1); //turn off led
        } else {
            heartbeat = true;
            gpio_set(GPIO_LED1,0);
        }
        thread_sleep(500);
    }
    return 0;
}




void target_init(void)
{
    nrf51_debug_init();
    dprintf(SPEW,"Target: PCA10000 DK...\n");

    ble1.radio_handle = (uint32_t *)NRF_RADIO;

    ble_initialize( &ble1 );


    if (NRF_RADIO->STATE == RADIO_STATE_STATE_Disabled) {
        dprintf(SPEW,"Radio presently disabled\n");
        dprintf(SPEW,"Ramping radio up in RX mode\n");
        NRF_RADIO->TASKS_RXEN = 1;
        dprintf(SPEW,"Waiting for RXIDLE...\n");
        while (NRF_RADIO->STATE != RADIO_STATE_STATE_RxIdle);
        dprintf(SPEW,"In State RxIdle\n");
    }



    blinkthread = thread_create( "blinky", &blinker , NULL, DEFAULT_PRIORITY, DEFAULT_STACK_SIZE );
    thread_resume(blinkthread);
}
