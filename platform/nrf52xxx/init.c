/*
 * Copyright (c) 2016 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lk/err.h>
#include <lk/debug.h>
#include <arch/arm/cm.h>
#include <dev/i2c.h>
#include <dev/uart.h>
#include <platform.h>
#include <platform/init.h>
#include <platform/clock.h>
#include <nrfx.h>
#include <mdk/system_nrf52.h>
#include <nrfx_clock.h>


static bool hf_early_state;
static int hf_src;
static bool lf_early_state;
static int lf_src;

void platform_early_init(void) {
    // Crank up the clock before initing timers.
    lf_early_state = nrfx_clock_is_running(NRF_CLOCK_DOMAIN_LFCLK, &lf_src);
    hf_early_state = nrfx_clock_is_running(NRF_CLOCK_DOMAIN_HFCLK, &hf_src);

    SystemInit();
    // Initialize the clock control peripheral
    nrf52_clock_init();

    arm_cm_systick_init(SystemCoreClock);
}

void platform_init(void) {

    dprintf(SPEW, "Nordic nrf52xxx platform for lk...\n");

    char *variant = (char*)&NRF_FICR->FICR_INFO.VARIANT;
    dprintf(SPEW, "Part Number - %x-%c%c%c%c\n", NRF_FICR->FICR_INFO.PART,
        variant[3], variant[2], variant[1], variant[0]);
    dprintf(SPEW, "\tRam: %dk\n", NRF_FICR->FICR_INFO.RAM);
    dprintf(SPEW, "\tFlash: %d pages of %d bytes each (%dk bytes total)\n", \
            NRF_FICR->CODESIZE, NRF_FICR->CODEPAGESIZE, \
            (NRF_FICR->CODESIZE * NRF_FICR->CODEPAGESIZE)>>10);
    dprintf(SPEW, "\tRadio MAC address  %02x:%02x:%02x:%02x:%02x:%02x\n", \
            (NRF_FICR->DEVICEADDR[1] >> 8) & 0xFF, \
            (NRF_FICR->DEVICEADDR[1]) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >> 24) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >> 16) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >>  8) & 0xFF, \
            (NRF_FICR->DEVICEADDR[0] >>  0) & 0xFF);

    // Note: i2c_init will only instantiate an i2c device if proper defines
    //   are set.  See comments at top of i2c_master.c for more info.
    i2c_init();
}
