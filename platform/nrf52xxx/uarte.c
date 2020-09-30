/*
 * Copyright (c) 2020 Eric Holland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <nrfx.h>
/*
  To use the uarte driver, NRFX_UARTE_ENABLED must be set to 1 in the GLOBAL_DEFINES
  additionally, NRFX_UARTE0_ENABLED and/or NRFX_UARTE1_ENABLED must also be 1 to specify
  which modules are active.  These would ideally be set in either a project rule.mk
  or in a targets rules.mk.

  The pins to be used for each of the active TWIM modules should be defined as:
  TWIM0_SCL_PIN, TWIM0_SDA_PIN  (if twim0 is used)
  TWIM1_SCL_PIN, TWIM1_SDA_PIN  (if twim1 is used)
  and ideally be defined in the targets include/gpioconfig.h since it is included here.
*/

#if (NRFX_UARTE_ENABLED)
#include <nrfx_log.h>
#include <nrfx_uarte.h>

#include <dev/uart.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <target/gpioconfig.h>


#define UARTE_TIMEOUT_MS 1000

typedef struct uarte_dev {
    nrfx_uarte_t uarte;
    event_t evt;
    mutex_t lock;
    nrfx_uarte_evt_type_t result;
} uarte_dev_t;

#if (NRFX_UARTE0_ENABLED)
static uarte_dev_t uarte0 = { NRFX_UARTE_INSTANCE(0),
                              EVENT_INITIAL_VALUE(uarte0.evt, false, 0),
                              MUTEX_INITIAL_VALUE(uarte0.lock),
                              0
                            };

void nrf52_UARTE0_UART0_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_uarte_0_irq_handler();
    arm_cm_irq_exit(true);
}
#endif


#if (NRFX_UARTE1_ENABLED)
static uarte_dev_t uarte1 = { NRFX_UARTE_INSTANCE(1),
                              EVENT_INITIAL_VALUE(uarte1.evt, false, 0),
                              MUTEX_INITIAL_VALUE(uarte1.lock),
                              0
                            };

void nrf52_UARTE1_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_uarte_1_irq_handler();
    arm_cm_irq_exit(true);
}
#endif

static inline uarte_dev_t *get_nrfx_uarte(int instance) {
    switch (instance) {
#if (NRFX_UARTE0_ENABLED)
        case 0:
            return &uarte0;
#endif
#if (NRFX_UARTE1_ENABLED)
        case 1:
            return &uarte1;
#endif
        default:
            return NULL;
    }
}

void uarte_evt_handler(nrfx_uarte_event_t const *p_event,void *p_context) {
    uarte_dev_t *uarte = (uarte_dev_t *)p_context;
    uarte->result = p_event->type;
    event_signal(&uarte->evt, false);
}

void uarte_init_early(void) {}

void uarte_init(void) {
    nrfx_err_t status;

#if (NRFX_UARTE0_ENABLED)
    //Pins should be defined in target/gpioconfig.h
    nrfx_uarte_config_t uarte0_config = NRFX_UARTE_DEFAULT_CONFIG(UARTE0_TX_PIN, UARTE0_RX_PIN);
    uarte0_config.p_context = &uarte0;

    status = nrfx_uarte_init(&uarte0.uarte, &uarte0_config, uarte_evt_handler);
    if (status != NRFX_SUCCESS) {
        NRFX_LOG_ERROR("ERROR in uarte0 init:%s \n",NRFX_LOG_ERROR_STRING_GET(status));
    }
#endif

#if (NRFX_UARTE1_ENABLED)
    //Pins should be defined in target/gpioconfig.h
    nrfx_uarte_config_t uarte1_config = NRFX_UARTE_DEFAULT_CONFIG(UARTE1_TX_PIN, UARTE1_RX_PIN);
    uarte1_config.p_context = &uarte1;

    status = nrfx_uarte_init(&uarte1.uarte, &uarte1_config, uarte_evt_handler);
    if (status != NRFX_SUCCESS) {
        NRFX_LOG_ERROR("ERROR in uarte1 init:%s \n",NRFX_LOG_ERROR_STRING_GET(status));
    }
#endif
}





#if 0

static status_t i2c_xfer(int bus, nrfx_twim_xfer_desc_t *desc) {
    twim_dev_t *twim = get_nrfx_twim(bus);
    if (twim == NULL) return ERR_INVALID_ARGS;

    mutex_acquire(&twim->lock);

    nrfx_err_t nrfx_status = nrfx_twim_xfer(&twim->twim, desc, 0);

    event_wait_timeout(&twim->evt, TWIM_MASTER_TIMEOUT_MS);
    event_unsignal(&twim->evt);
    mutex_release(&twim->lock);

    if (nrfx_status != NRFX_SUCCESS) {
        NRFX_LOG_ERROR("%s:%s \n", __func__, NRFX_LOG_ERROR_STRING_GET(nrfx_status));
        return (status_t)nrfx_status;
    }
    return NO_ERROR;
}

status_t i2c_transmit(int bus, uint8_t address, const void *buf, size_t count) {

    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TX(address, (uint8_t *)buf, count);

    return i2c_xfer(bus, &xfer);
}

status_t i2c_receive(int bus, uint8_t address, void *buf, size_t count) {

    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_RX(address, (uint8_t *)buf, count);

    return i2c_xfer(bus, &xfer);
}

status_t i2c_write_reg_bytes(int bus, uint8_t address, uint8_t reg, const uint8_t *val,
                             size_t cnt) {

    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXTX(address, &reg, 1, (uint8_t *)val, cnt);

    return i2c_xfer(bus, &xfer);
}

status_t i2c_read_reg_bytes(int bus, uint8_t address, uint8_t reg, uint8_t *val, size_t cnt) {
    nrfx_twim_xfer_desc_t xfer = NRFX_TWIM_XFER_DESC_TXRX(address, &reg, 1, (uint8_t *)val, cnt);

    return i2c_xfer(bus, &xfer);
}
#endif
#endif // NRFX_TWIM_ENABLED
