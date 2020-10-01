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

#ifndef NRFX_UARTE_TX_BUFF_SIZE
#define NRFX_UARTE_TX_BUFF_SIZE 128
#endif
#ifndef NRFX_UARTE_RX_BUFF_SIZE
#define NRFX_UARTE_RX_BUFF_SIZE 128
#endif

#include <nrfx_log.h>
#include <nrfx_uarte.h>

#include <dev/uart.h>
#include <kernel/event.h>
#include <kernel/mutex.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lib/cbuf.h>
#include <target/gpioconfig.h>

#define UARTE_TIMEOUT_MS 1000

typedef struct uarte_dev {
    nrfx_uarte_t uarte;
    event_t evt;
    mutex_t lock;
    cbuf_t tx_buf;
    cbuf_t rx_buf;
    nrfx_uarte_evt_type_t result;
} uarte_dev_t;


#if (NRFX_UARTE0_ENABLED)

void nrf52_UARTE0_UART0_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_uarte_0_irq_handler();
    arm_cm_irq_exit(true);
}
#endif

#if (NRFX_UARTE1_ENABLED)

void nrf52_UARTE1_IRQ(void) {
    arm_cm_irq_entry();
    nrfx_uarte_1_irq_handler();
    arm_cm_irq_exit(true);
}
#endif

static inline status_t nrfx_uarte_init_instance(nrfx_uarte_t *uarte, int instance) {
    switch (instance) {
#if (NRFX_UARTE0_ENABLED)
        case 0:
            uarte->p_reg = NRF_UARTE0;
            uarte->drv_inst_idx = NRFX_UARTE0_INST_IDX;
            return NO_ERROR;
#endif
#if (NRFX_UARTE1_ENABLED)
        case 1:
            uarte->p_reg = NRF_UARTE1;
            uarte->drv_inst_idx = NRFX_UARTE1_INST_IDX;
            return NO_ERROR;
#endif
        default:
            return -1;
    }
}

void uarte_evt_handler(nrfx_uarte_event_t const *p_event,void *p_context) {
    uarte_dev_t *uarte = (uarte_dev_t *)p_context;
    uarte->result = p_event->type;
    event_signal(&uarte->evt, false);
}

void uarte_init_early(void) {}

status_t uarte_init(uarte_dev_t *uarte, int instance, int tx_pin, int rx_pin) {
    ASSERT(uarte);
    nrfx_err_t status;
    if (nrfx_uarte_init_instance(&uarte->uarte, instance) != NO_ERROR) {
      return ERR_NOT_FOUND;
    }
    event_init(&uarte->evt, false, 0);
    mutex_init(&uarte->lock);
    cbuf_initialize(&uarte->tx_buf, NRFX_UARTE_TX_BUFF_SIZE);
    cbuf_initialize(&uarte->rx_buf, NRFX_UARTE_RX_BUFF_SIZE);

    nrfx_uarte_config_t uarte_config = NRFX_UARTE_DEFAULT_CONFIG(tx_pin, rx_pin);
    uarte_config.p_context = uarte;

    status = nrfx_uarte_init(&uarte->uarte, &uarte_config, uarte_evt_handler);
    if (status != NRFX_SUCCESS) {
        NRFX_LOG_ERROR("ERROR in uarte0 init:%s \n",NRFX_LOG_ERROR_STRING_GET(status));
        return (status_t) status;
    }
    return NO_ERROR;
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
