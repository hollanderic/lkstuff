/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

/*
 *  LK driver for Nordic RTC peripheral.  Uses Nordic nrfx library.
 */

#include <nrfx.h>

#if (NRFX_RTC_ENABLED)
#include <nrfx_rtc.h>
#include <lk/err.h>

#include <sys/types.h>

#include <platform/rtc.h>


#if (NRFX_RTC0_ENABLED)
static lk_rtc_dev_t rtc0 = { NRFX_RTC_INSTANCE(0),
                             { NULL, NULL, NULL, NULL},
                             { NULL, NULL, NULL, NULL},
                           };

void nrf52_RTC0_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_rtc_0_irq_handler();
    arm_cm_irq_exit(true);
}

// Called in interrupt context
void nrf52_rtc0_handler(nrfx_rtc_int_type_t int_type) {

}
#endif


static inline lk_rtc_dev_t *get_nrfx_rtc(int instance) {
    switch (instance) {
#if (NRFX_RTC0_ENABLED)
        case 0:
            return &rtc0;
#endif
#if (NRFX_RTC1_ENABLED)
        case 1:
            return &rtc1;
#endif
#if (NRFX_RTC2_ENABLED)
        case 2:
            return &rtc2;
#endif
        default:
            return NULL;
    }
}

status_t nrf52_rtc_init(lk_rtc_instances_t instance) {
    lk_rtc_dev_t *dev = get_nrfx_rtc(instance);
    if (dev == NULL) {
      dprintf(CRITICAL, "NRFX_RTC%d not available\n",instance);
      return ERR_NOT_FOUND;
    }


    return NO_ERROR;
}


#endif  // NRFX_RTC_ENABLED

