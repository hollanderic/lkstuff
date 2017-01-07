/*
 * Copyright (c) 2016 Eric Holland
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

#include <lib/ble.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>
#include <dev/ble_radio.h>

void ble_dump_packet(ble_t *ble_p);

static int _ble_run(void *arg){

    ble_t *ble_p = (ble_t *)arg;

    while (1) {
        //do magical ble shit here

        if ( mutex_acquire_timeout(&(ble_p->lock),0) == NO_ERROR )
        {
            switch(ble_p->state) {
                case BLE_IDLE:
                    //ble_dump_packet(ble_p);
                    break;
                default:
                    break;
            }
        }

        mutex_release(&(ble_p->lock));
		thread_sleep(1000);

	}
	return 0;
}



/*
    Sets up state for ADV_NONCONN_IND advertising.
*/
ble_status_t ble_init_adv_nonconn_ind( ble_t *ble_p){

    ble_p->channel_index = 37;

    ble_p->access_address   = BLE_ACCESSADDRESS_ADVERTISING;
    ble_p->pdu_type         = PDU_ADV_NONCONN_IND;
    //TODO - need to support random address as well
    ble_p->hw_addr_type = HW_ADDR_TYPE_PUBLIC;

    ble_p->payload[0] = ble_p->hw_addr[0];
    ble_p->payload[1] = ble_p->hw_addr[1];
    ble_p->payload[2] = ble_p->hw_addr[2];
    ble_p->payload[3] = ble_p->hw_addr[3];
    ble_p->payload[4] = ble_p->hw_addr[4];
    ble_p->payload[5] = ble_p->hw_addr[5];

    ble_p->payload_length = 6;

    return BLE_NO_ERROR;
}

uint8_t _ble_remaining_pdu(ble_t *ble_p) {

    if ( (ble_p->pdu_type & PDU_TYPE_MASK) == PDU_TYPE_ADV) {
        return  BLE_MAX_ADV_PDU_SIZE - ble_p->payload_length;
    } else {
        return  BLE_MAX_DATA_PDU_SIZE - ble_p->payload_length;
    }
}

ble_status_t ble_go_idle(ble_t *ble_p){

    uint32_t status;

    status = ble_radio_shutdown();
    ble_p->state = BLE_IDLE;

    return status;
}

ble_status_t ble_gap_add_flags(ble_t * ble_p){
   BLE_CHECK_AND_LOCK(ble_p);

    if ( _ble_remaining_pdu(ble_p) < 3 ){
        BLE_UNLOCK(ble_p);
        return BLE_ERR_PDU_FULL;
    }
    ble_p->payload[ ble_p->payload_length     ] = 2;
    ble_p->payload[ ble_p->payload_length + 1 ] = GAP_ADTYPE_FLAGS;
    ble_p->payload[ ble_p->payload_length + 2 ] = 0x06;
    ble_p->payload_length +=3;

    BLE_UNLOCK(ble_p);

    return BLE_NO_ERROR;
}

ble_status_t ble_gap_add_shortname(ble_t *ble_p, uint8_t * str, uint8_t len){

    BLE_CHECK_AND_LOCK(ble_p);

    if ( _ble_remaining_pdu(ble_p) < ( len + 2 )) {
        BLE_UNLOCK(ble_p);
        return BLE_ERR_PDU_FULL;
    }
    ble_p->payload[ ble_p->payload_length     ] = len + 1;
    ble_p->payload[ ble_p->payload_length + 1 ] = GAP_ADTYPE_LOCAL_NAME_SHORT;
    ble_p->payload_length +=2;

    for (int i=0 ; i < len ; i++) {
        ble_p->payload[ ble_p->payload_length + i ] = str[i];
    }
    ble_p->payload_length += len;

    BLE_UNLOCK(ble_p);

    return BLE_NO_ERROR;

}


ble_status_t ble_gap_add_service_data_128(ble_t *ble_p, uint8_t * uuid, uint32_t data){

    BLE_CHECK_AND_LOCK(ble_p);

    if ( _ble_remaining_pdu(ble_p) < (  22 )) {
        BLE_UNLOCK(ble_p);
        return BLE_ERR_PDU_FULL;
    }
    ble_p->payload[ ble_p->payload_length     ] = 20 + 1;
    ble_p->payload[ ble_p->payload_length + 1 ] = 0x21;//GAP_ADTYPE_SERVICE_DATA;

    for (int i = 0; i < 16; i++) {
        ble_p->payload[ ble_p->payload_length + i + 2 ] = uuid[i];
    }
    ble_p->payload[ ble_p->payload_length + 18 ] = data & 0x000000ff;
    ble_p->payload[ ble_p->payload_length + 19 ] = (data >> 8) & 0x000000ff;
    ble_p->payload[ ble_p->payload_length + 20 ] = (data >>16) & 0x000000ff;
    ble_p->payload[ ble_p->payload_length + 21 ] = (data >>24) & 0x000000ff;

    ble_p->payload_length += 22;

    BLE_UNLOCK(ble_p);

    return BLE_NO_ERROR;

}

void ble_initialize( ble_t *ble_p ) {

    mutex_init( &(ble_p->lock) );
    ble_p->access_address   = BLE_ACCESSADDRESS_ADVERTISING;
    ble_p->pdu_type         = PDU_ADV_NONCONN_IND;
    ble_p->payload_length = 0;
    ble_p->state = BLE_IDLE;

    ble_radio_initialize( ble_p );

    ble_get_hw_addr( ble_p );

}


void ble_dump_packet(ble_t *ble_p){

    uint16_t pdu_header = ( 0x55 << 8) + ble_p->payload_length;

    printf("%02x --- %02x ---",ble_p->payload[-2], ble_p->payload[-1]);
    for (int i = 0; i < ble_p->payload_length ; i++)
        printf("%02x", ble_p->payload[i]);
    printf("\n");

}





