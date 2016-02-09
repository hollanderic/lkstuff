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

#include <platform/nrf51.h>
#include <lib/ble.h>
#include <kernel/event.h>
#include <arch/arm/cm.h>
#include <dev/ble_radio.h>
#include <platform/nrf51_radio.h>
#include <target/gpioconfig.h>
#include <dev/gpio.h>

#include <platform/gpio.h>


#define PCNF0_ADV_PDU       6 << RADIO_PCNF0_LFLEN_Pos | \
                            1 << RADIO_PCNF0_S0LEN_Pos | \
                            2 << RADIO_PCNF0_S1LEN_Pos
#define PCNF0_DATA_PDU      5 << RADIO_PCNF0_LFLEN_Pos | \
                            1 << RADIO_PCNF0_S0LEN_Pos | \
                            3 << RADIO_PCNF0_S1LEN_Pos


static nrf_packet_buffer_t nrf_packet_buffer;
static event_t radio_evt;


void nrf51_RADIO_IRQ (void) {

    arm_cm_irq_entry();

    NRF_RADIO->EVENTS_DISABLED = 0;

    event_signal(&radio_evt, false);

    gpio_set(GPIO_LED1,1);

    arm_cm_irq_exit(true);
}


void radio_dump_packet(){

    printf(" %08x - %08x - %08x -", nrf_packet_buffer.s0, nrf_packet_buffer.length, nrf_packet_buffer.s1);
    for (int i = 0; i < nrf_packet_buffer.length ; i++)
        printf("%02x", nrf_packet_buffer.data[i]);
    printf("\n");

}



/*
    Initialize the radio to ble mode, place in idle.
    This will perform necessary checks to ensure configuration
    is viable to ramp up for transmission or reception.

*/
void ble_radio_initialize(ble_t *ble_p) {

    event_init(&radio_evt,false, EVENT_FLAG_AUTOUNSIGNAL);

    ble_p->radio_event = &radio_evt;

    NRF_CLOCK->TASKS_HFCLKSTART = 1;    //Start HF xtal oscillator (required for radio)


    NRF_RADIO->POWER =1;

    if ((NRF_FICR->OVERRIDEEN & FICR_OVERRIDEEN_BLE_1MBIT_Msk) == \
                    (FICR_OVERRIDEEN_BLE_1MBIT_Override << FICR_OVERRIDEEN_BLE_1MBIT_Pos))
    {
        NRF_RADIO->OVERRIDE0 = NRF_FICR->BLE_1MBIT[0];
        NRF_RADIO->OVERRIDE1 = NRF_FICR->BLE_1MBIT[1];
        NRF_RADIO->OVERRIDE2 = NRF_FICR->BLE_1MBIT[2];
        NRF_RADIO->OVERRIDE3 = NRF_FICR->BLE_1MBIT[3];
        NRF_RADIO->OVERRIDE4 = NRF_FICR->BLE_1MBIT[4] | \
                (RADIO_OVERRIDE4_ENABLE_Enabled << RADIO_OVERRIDE4_ENABLE_Pos);
    }

    NRF_RADIO->CRCCNF =     RADIO_CRCCNF_LEN_Three      << RADIO_CRCCNF_LEN_Pos  | \
                            RADIO_CRCCNF_SKIPADDR_Skip  << RADIO_CRCCNF_SKIPADDR_Pos;

    NRF_RADIO->CRCPOLY      =   BLE_CRC_POLYNOMIAL;
    NRF_RADIO->TXPOWER      =   RADIO_TXPOWER_TXPOWER_Pos4dBm << RADIO_TXPOWER_TXPOWER_Pos;
    NRF_RADIO->MODE         =   RADIO_MODE_MODE_Ble_1Mbit << RADIO_MODE_MODE_Pos;

    NRF_RADIO->PCNF0        =   6 << RADIO_PCNF0_LFLEN_Pos | \
                                            1 << RADIO_PCNF0_S0LEN_Pos | \
                                            2 << RADIO_PCNF0_S1LEN_Pos;

    NRF_RADIO->PCNF1        =   255 << RADIO_PCNF1_MAXLEN_Pos   | \
                                            3   << RADIO_PCNF1_BALEN_Pos | \
                                            RADIO_PCNF1_ENDIAN_Little << RADIO_PCNF1_ENDIAN_Pos | \
                                            RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos;

    NRF_RADIO->BASE0        =   (ble_p->access_address << 8) & 0xffffff00;
    NRF_RADIO->PREFIX0      =   (ble_p->access_address >> 24) & 0xff;


    ble_p->payload.buff = nrf_packet_buffer.data;

    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
}

/*
    Configures all the tx mechanisms in preparation for TX.
        This includes channel, whitening init, length fields, etc.
        Assumes that the payload is already laid out in RAM buffer.
*/
void ble_radio_tx(ble_t * ble_p){
    //TODO - check channel range
    //TODO - check payload length
    //TODO - eliminate use of s1 field (expand length)

    NRF_RADIO->EVENTS_END   =   0;
    NRF_RADIO->EVENTS_READY =   0;

    nrf_packet_buffer.s0        = ble_p->pdu_type & PDU_TYPE_MASK;

    if (ble_p->hw_addr_type == HW_ADDR_TYPE_RANDOM)
            nrf_packet_buffer.s0 = nrf_packet_buffer.s0 | 0x40;

    nrf_packet_buffer.length    = ble_p->payload_length;
    nrf_packet_buffer.s1        = 0;
    NRF_RADIO->PACKETPTR    =   (uint32_t )&nrf_packet_buffer;
    NRF_RADIO->BASE0        =   (ble_p->access_address << 8) & 0xffffff00;
    NRF_RADIO->PREFIX0      =   (ble_p->access_address >> 24) & 0xff;
    NRF_RADIO->TXADDRESS    =   0;
    NRF_RADIO->CRCINIT      =   BLE_CRC_INITIAL_ADV;
    NRF_RADIO->DATAWHITEIV  =   ble_p->channel;

    uint8_t ch;
    switch (ble_p->channel) {
        case 37:
            ch = 0;
            break;
        case 38:
            ch = 12;
            break;
        default:
            ch = ble_p->channel;
    }
    NRF_RADIO->FREQUENCY    =   (ch << 1) + 2;
    NRF_RADIO->SHORTS       =   RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos | \
                                            RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos;

    NRF_RADIO->EVENTS_DISABLED  =   0;
    NRF_RADIO->INTENSET         =   RADIO_INTENSET_DISABLED_Enabled << RADIO_INTENSET_DISABLED_Pos;
    gpio_set(GPIO_LED1,0);
    NRF_RADIO->TASKS_TXEN       =   1;
    event_wait_timeout(ble_p->radio_event, 2000);  //todo, check for timeout and bomb.
}






void ble_radio_start_rx(ble_t * ble_p){


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




