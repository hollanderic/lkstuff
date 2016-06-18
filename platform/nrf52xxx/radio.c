/*
 * Copyright (c) 2015 Eric Holland
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

#include <platform/nrf52.h>
#include <lib/ble.h>
#include <kernel/event.h>
#include <arch/arm/cm.h>
#include <dev/ble_radio.h>
#include <platform/nrf_radio.h>
#include <target/gpioconfig.h>
#include <dev/gpio.h>

#include <platform/gpio.h>

#define NUM_RX_BUFFERS 2
#define RX_BUFFER_LENGTH 64

#define PCNF0_ADV_PDU       6 << RADIO_PCNF0_LFLEN_Pos | \
                            1 << RADIO_PCNF0_S0LEN_Pos | \
                            2 << RADIO_PCNF0_S1LEN_Pos
#define PCNF0_DATA_PDU      5 << RADIO_PCNF0_LFLEN_Pos | \
                            1 << RADIO_PCNF0_S0LEN_Pos | \
                            3 << RADIO_PCNF0_S1LEN_Pos


static uint8_t nrf_tx_buffer[64];


static uint8_t nrf_rx_buffer[NUM_RX_BUFFERS * RX_BUFFER_LENGTH];
static uint8_t curr_rx_buff;

static event_t radio_end_evt;
static event_t radio_disabled_evt;

static uint32_t testvar;

void nrf_evt_timeout( event_t * event_p, uint32_t delay_ticks);
void ble_radio_start_rx(ble_t * ble_p);


void nrf52_RADIO_IRQ (void) {

    arm_cm_irq_entry();
    //gpio_set(GPIO_LED1,0);
    if ((NRF_RADIO->INTENSET & RADIO_INTENSET_END_Msk ) && NRF_RADIO->EVENTS_END ) {
        NRF_RADIO->EVENTS_END = 0;
        event_signal(&radio_end_evt, false);
        //gpio_set(GPIO_LED1,1);

    }

    if ((NRF_RADIO->INTENSET & RADIO_INTENSET_DISABLED_Msk ) && NRF_RADIO->EVENTS_DISABLED ) {
        NRF_RADIO->EVENTS_DISABLED = 0;
        event_signal(&radio_disabled_evt, false);
        testvar=NRF_RTC1->COUNTER;
    }

    //gpio_set(GPIO_LED1,1);
    arm_cm_irq_exit(true);
}




/*
 *  gracefully waits for radio to enter disabled state, which is the starting point for
 *      any radio activity when swtiching between rx and tx mode
 */
static inline uint32_t _wait_radio_disabled(void) {
    if (NRF_RADIO->STATE != RADIO_STATE_STATE_Disabled) {       // Only yield the thread if we need to
        NRF_RADIO->TASKS_DISABLE = 1;       // Just to be safe, hit the stop button
        event_wait_timeout(&radio_disabled_evt,10);
        return 0;
    }
    event_unsignal(&radio_disabled_evt); // Unsignal the event in case we had a race
    return 0;
}

uint32_t ble_radio_shutdown(void){
    return _wait_radio_disabled();
}

/*
    Initialize the radio to ble mode, place in idle.
    This will perform necessary checks to ensure configuration
    is viable to ramp up for transmission or reception.

*/
void ble_radio_initialize(ble_t *ble_p) {

    event_init(&radio_end_evt,false, EVENT_FLAG_AUTOUNSIGNAL);
    event_init(&radio_disabled_evt, false, EVENT_FLAG_AUTOUNSIGNAL);

    NRF_CLOCK->TASKS_HFCLKSTART = 1;    //Start HF xtal oscillator (required for radio)

    NRF_RADIO->POWER =1;

    NRF_RADIO->CRCCNF =     RADIO_CRCCNF_LEN_Three      << RADIO_CRCCNF_LEN_Pos  | \
                            RADIO_CRCCNF_SKIPADDR_Skip  << RADIO_CRCCNF_SKIPADDR_Pos;

    NRF_RADIO->CRCPOLY      =   BLE_CRC_POLYNOMIAL;
    NRF_RADIO->TXPOWER      =   RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos;
    NRF_RADIO->MODE         =   RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos;

    NRF_RADIO->PCNF0        =   8   << RADIO_PCNF0_LFLEN_Pos | \
                                1   << RADIO_PCNF0_S0LEN_Pos | \
                                0   << RADIO_PCNF0_S1LEN_Pos;

    NRF_RADIO->PCNF1        =   50                          << RADIO_PCNF1_MAXLEN_Pos   | \
                                3                           << RADIO_PCNF1_BALEN_Pos    | \
                                RADIO_PCNF1_ENDIAN_Little   << RADIO_PCNF1_ENDIAN_Pos   | \
                                RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos;

    NRF_RADIO->CRCINIT      =   BLE_CRC_INITIAL_ADV;

    ble_p->payload          =   &(nrf_tx_buffer[2]);  //skip the header, may need to change this later and factor into gap commands

    NRF_RADIO->EVENTS_END   =   0;
    NRF_RADIO->INTENSET     =   RADIO_INTENSET_END_Enabled << RADIO_INTENSET_END_Pos | \
                                RADIO_INTENSET_DISABLED_Enabled << RADIO_INTENSET_DISABLED_Pos;

    NRF_RADIO->SHORTS       =   RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos | \
                                RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos;

    NRF_RADIO->RXADDRESSES = 1UL;
    NRF_RADIO->TXADDRESS = 0UL;

    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}

void _ble_radio_pre_init(ble_t * ble_p){

    NRF_RADIO->DATAWHITEIV  =   ble_p->channel_index;

    uint8_t ch;
    switch (ble_p->channel_index) {
        case 37:
            ch = 0;
            break;
        case 38:
            ch = 12;
            break;
        default:
            ch = ble_p->channel_index;
    }

    NRF_RADIO->FREQUENCY    =   (ch << 1) + 2;
    NRF_RADIO->BASE0        =   (ble_p->access_address << 8 ) & 0xffffff00;
    NRF_RADIO->PREFIX0      =   (ble_p->access_address >> 24) & 0xff;
}


/*
    Configures all the tx mechanisms in preparation for TX.
        This includes channel, whitening init, length fields, etc.
        Assumes that the payload is already laid out in RAM buffer.
*/
uint32_t ble_radio_tx(ble_t * ble_p){
    //TODO - check channel range
    //TODO - check payload length

    nrf_tx_buffer[0]        = ble_p->pdu_type & PDU_TYPE_MASK;
    if (ble_p->hw_addr_type == HW_ADDR_TYPE_RANDOM)
            nrf_tx_buffer[0] |= 0x40;

    nrf_tx_buffer[1]        = ble_p->payload_length;

    _wait_radio_disabled();

    NRF_RADIO->PACKETPTR    =   (uint32_t )&nrf_tx_buffer;

    _ble_radio_pre_init(ble_p);

    gpio_set(GPIO_LED1,0);
    NRF_RADIO->TASKS_TXEN       =   1;
    event_wait_timeout(&radio_end_evt, 10);  //todo, check for timeout and bomb. TODO- really need to make this tickless
    if ((ble_p->scannable) ) {
        // set up the receive
        ble_radio_start_rx(ble_p);


    }
    return 0;
}
/*
    Moves to the next rx buffer in sequence and sets packetptr;
*/
static inline void _ble_radio_increment_rx_buffer(void) {
    curr_rx_buff = (curr_rx_buff +1) % (NUM_RX_BUFFERS);
    NRF_RADIO->PACKETPTR = (uint32_t)nrf_rx_buffer + curr_rx_buff*RX_BUFFER_LENGTH;
}

/*
    loads pointer and length of current active packet buffer in ble state struct
*/
static inline void _ble_get_current_rx_buffer(ble_t * ble_p){
    ble_p->payload = &nrf_rx_buffer[ curr_rx_buff * RX_BUFFER_LENGTH ];
    ble_p->payload_length = ble_p->payload[1] % (RX_BUFFER_LENGTH -2); //length field doesn't include PDU header
    ble_p->payload[1] = ble_p->payload_length;  //Keep bad length field from allowing a buffer overrun up the stack
}



void _ble_radio_scan_init(ble_t * ble_p){

    NRF_RADIO->SHORTS = RADIO_SHORTS_DISABLED_RXEN_Msk  | \
                        RADIO_SHORTS_END_DISABLE_Msk    | \
                        RADIO_SHORTS_READY_START_Msk    ;  //Continuous rx looping
    curr_rx_buff = 0;
    NRF_RADIO->PACKETPTR    =   (uint32_t)&nrf_rx_buffer;
    ble_p->state = BLE_SCANNING;
}



/*
    ble_radio_scan_continuous - used for continuous scanning.
        return val = zero if packet received
        return val < 0 if due to timeout

    Uses double buffered rx buffers, returns with ble_p->payload pointing
    to the buffer, null if due to timeout.

    Will return, but radio immediately goes back into receive loop.
*/
uint32_t ble_radio_scan_continuous(ble_t * ble_p, lk_time_t timeout){
    uint32_t retval;
    if ((ble_p->state != BLE_SCANNING)) {
        printf("Initing scan mode\n");
        _wait_radio_disabled();
        _ble_radio_scan_init(ble_p);
        _ble_radio_pre_init(ble_p);
        NRF_RADIO->TASKS_RXEN = 1;
    }
    retval = event_wait_timeout(&radio_end_evt, timeout);
    if (retval != 0) {
        ble_p->payload = NULL;
        ble_p->payload_length = 0;
        _ble_radio_increment_rx_buffer();
        return -1;
    } else {
        _ble_get_current_rx_buffer(ble_p);
        _ble_radio_increment_rx_buffer();
        return 0;
    }
}

void ble_radio_start_rx(ble_t * ble_p){
    uint32_t d0, d1;
    _wait_radio_disabled();
    nrf_evt_timeout(&radio_end_evt,50);
    NRF_RADIO->PACKETPTR    =   (uint32_t)&nrf_rx_buffer;
    NRF_RADIO->TASKS_RXEN   =   1;
    uint32_t i = event_wait_timeout(&radio_end_evt,4000);

    if (NRF_RADIO->STATE == RADIO_STATE_STATE_Rx ) { //we didn't get anything
        NRF_RADIO->TASKS_DISABLE = 1;   // shut down radio
    } else {
        for (int i = 0 ; i < (nrf_rx_buffer[1]+2) ; i++) {
            printf("%02X ",nrf_rx_buffer[i]);
        }
        printf("\n");
    }
    return 0;

}

/*
    loads hw addr(mac) and type.

*/
int32_t ble_get_hw_addr(ble_t *ble_p) {

    ble_p->hw_addr[0] = ( NRF_FICR->DEVICEADDR[0] >> 0  ) & 0xff;
    ble_p->hw_addr[1] = ( NRF_FICR->DEVICEADDR[0] >> 8  ) & 0xff;
    ble_p->hw_addr[2] = ( NRF_FICR->DEVICEADDR[0] >> 16 ) & 0xff;
    ble_p->hw_addr[3] = ( NRF_FICR->DEVICEADDR[0] >> 24 ) & 0xff;
    ble_p->hw_addr[4] = ( NRF_FICR->DEVICEADDR[1] >> 0  ) & 0xff;
    ble_p->hw_addr[5] = ( NRF_FICR->DEVICEADDR[1] >> 8  ) & 0xff;

    ble_p->hw_addr_type = HW_ADDR_TYPE_RANDOM;

    return 0;
}
/*
    Set the channel, takes the RF channel index number (2402 = channel index 0)
        also sets the whitening initial value based on the channel index
*/
int32_t ble_radio_set_channel(ble_t * ble_p, uint8_t channel){

    NRF_RADIO->FREQUENCY = (channel << 1) + 2;
    NRF_RADIO->DATAWHITEIV = channel;

    return 0;
}




