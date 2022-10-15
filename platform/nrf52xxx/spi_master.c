/*
 * Copyright (c) 2022 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <nrfx.h>
/*
  To use the spi master driver, NRFX_SPIM_ENABLED must be set to 1 in the GLOBAL_DEFINES
  additionally, NRFX_SPIM[0..3]_ENABLED must also be 1 to specify
  which instances are active.  These would ideally be set in either a project rule.mk
  or in a targets rules.mk.  Be mindful that not all SPIM instances support the same
  sets of features.

  The pins to be used for each of the active SPIM modules should be defined as:
  SPIM0_SCL_PIN, TWIM0_SDA_PIN  (if twim0 is used)
  TWIM1_SCL_PIN, TWIM1_SDA_PIN  (if twim1 is used)
  and ideally be defined in the targets include/gpioconfig.h since it is included here.
*/

#if (NRFX_SPIM_ENABLED)
#include <nrfx_log.h>
#include <nrfx_spim.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <target/gpioconfig.h>

static inline bool is_nrfx_spim_enabled(int bus) {
    switch (bus) {
        case 0:
            return NRFX_SPIM0_ENABLED;
        case 1:
            return NRFX_SPIM1_ENABLED;
        case 2:
            return NRFX_SPIM2_ENABLED;
        case 3:
            return NRFX_SPIM3_ENABLED;
        default:
            return false;
    }
}

#if (NRFX_SPIM0_ENABLED)
#if (NRFX_TWIM0_ENABLED || NRFX_SPIS0_ENABLED || NRFX_TWIS0_ENABLED || NRFX_SPI0_ENABLED || NRFX_TWI0_ENABLED)
#error "NRFX_SPIM0 can't be used with TWIM0, SPIS0, TWIS0, SPI0, or TWI0"
#endif
static nrfx_spim_t spim0 = NRFX_SPIM_INSTANCE(0);


void spim0_handler(nrfx_spim_evt_t const * p_event, void *p_context) {

}

void nrf52_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_spim_0_irq_handler();
    arm_cm_irq_exit(true);
}

#endif




uint32_t spim_init(uint32_t bus, nrfx_spim_config_t config) {
    // Check to see if the appropriate SPIM instance is enabled in this build(i.e. - NRFX_SPIM0_ENABLED=1)
    ASSERT(is_nrfx_spim_enabled(bus));


    nrfx_err_t status = nrfx_spim_init(&spim0, &config, spim0_handler, &spim0);

    return 0;
}






#endif //NRFX_SPIM_ENABLED