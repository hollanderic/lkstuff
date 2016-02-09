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
#include <kernel/thread.h>
#include <lib/ble.h>
#include <platform/nrf51.h>
#include <platform/nrf51_radio.h>
#include <dev/ble_radio.h>


//   UUID  - 080223be-181a-4f59-b74a-ea5d04af35bc

void ble_start(void);
static void ble_init(const struct app_descriptor *app);

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>

STATIC_COMMAND_START
STATIC_COMMAND("ble_tests", "test ble", (console_cmd)&ble_start)
STATIC_COMMAND_END(bletests);

#endif


static ble_t ble1;
static thread_t *blethread;
static const char lkbeacon[] = "LKBeacon";

void ble_start(void) {




}

static int ble_run(void * args)
{

    ble_initialize( &ble1 );
    ble_init_adv_nonconn_ind(&ble1);
    ble_gatt_add_flags(&ble1);
    ble_gatt_add_shortname(&ble1, lkbeacon, sizeof(lkbeacon));

    while (1) {
        //if ( mutex_acquire_timeout(&(ble_p->lock),0) == NO_ERROR )

   		if (NRF_RADIO->STATE == RADIO_STATE_STATE_Disabled) {

        	ble1.channel = 37;
        	ble_radio_tx(&ble1);

		    ble1.channel = 38;
        	ble_radio_tx(&ble1);

		    ble1.channel = 39;
        	ble_radio_tx(&ble1);

	    }
	    thread_sleep(1000);
	}
	return 0;
}


static void ble_init(const struct app_descriptor *app) {

	blethread = thread_create("blethread", &ble_run, NULL, HIGH_PRIORITY, DEFAULT_STACK_SIZE);
	thread_resume(blethread);
}


APP_START(bletests)
	.init = ble_init,
	.flags = 0,
APP_END

