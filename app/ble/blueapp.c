/*
 * Copyright (c) 2016 Eric Holland
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
#include <app.h>
#include <debug.h>
#include <compiler.h>


#include <compiler.h>
#include <lib/ble.h>
#include <platform/nrf51.h>
#include <platform/nrf51_radio.h>
#include <dev/ble_radio.h>


void ble_start(void);
static void ble_init(const struct app_descriptor *app);

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

STATIC_COMMAND_START
STATIC_COMMAND("ble_tests", "test ble", (console_cmd)&ble_start)
STATIC_COMMAND_END(bletests);

#endif

void ble_start(void) {

}

static ble_t ble1;
static thread_t *blinkthread;
static const char lkbeacon[] = "LKBeacon";



static void ble_init(const struct app_descriptor *app)
{


    ble1.radio_handle = (uint32_t *)NRF_RADIO;

    ble_initialize( &ble1 );


    if (NRF_RADIO->STATE == RADIO_STATE_STATE_Disabled) {
        dprintf(SPEW,"Radio presently disabled\n");
        dprintf(SPEW,"Ramping radio up in TX mode\n");
        NRF_RADIO->TASKS_TXEN = 1;
        dprintf(SPEW,"Waiting for TXIDLE...\n");
        while (NRF_RADIO->STATE != RADIO_STATE_STATE_TxIdle);
        dprintf(SPEW,"In State TxIdle\n");
    }

    uint8_t i;

    //ble_start_beacon( &ble1 );

    while (1) {
	    ble_init_adv_nonconn_ind(&ble1);
	    ble_gatt_add_shortname(&ble1, lkbeacon, 4);

	    ble_radio_init_tx(&ble1);
	    radio_dump_packet();
	    dprintf(SPEW,"Starting tx...waiting for tx idle.  state=%d\n",NRF_RADIO->STATE);
	    NRF_RADIO->TASKS_START = 1;
	    while (NRF_RADIO->STATE != RADIO_STATE_STATE_Tx);

	    i=0;
	    while (NRF_RADIO->STATE != RADIO_STATE_STATE_TxIdle) {
	        i++;
	        //dprintf(SPEW,"state=%d\n",NRF_RADIO->STATE);
	    }
	    dprintf(SPEW,"%d\n",i);

	    ble1.channel = 38;
	    ble_radio_init_tx(&ble1);
	    NRF_RADIO->TASKS_START = 1;
	    i=0;
	    while (NRF_RADIO->STATE != RADIO_STATE_STATE_TxIdle) {
	        i++;
	        //dprintf(SPEW,"state=%d\n",NRF_RADIO->STATE);
	    }
	    dprintf(SPEW,"%d\n",i);

	    ble1.channel = 39;
	    ble_radio_init_tx(&ble1);
	    NRF_RADIO->TASKS_START = 1;
	    i=0;
	    while (NRF_RADIO->STATE != RADIO_STATE_STATE_TxIdle) {
	        i++;
	        //dprintf(SPEW,"state=%d\n",NRF_RADIO->STATE);
	    }
	    dprintf(SPEW,"%d\n",i);
	    dprintf(SPEW,"TX complete, all done!\n");
	}
}

APP_START(bletests)
	.init = ble_init,
	.flags = 0,
APP_END

