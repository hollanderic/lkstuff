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
#ifndef __LIB_BLE_H
#define __LIB_BLE_H

#include <err.h>
#include <kernel/thread.h>
#include <kernel/mutex.h>



typedef struct {

    uint8_t adv_addr[6];
    uint8_t adv_data[31];

} ind_packet_t;


typedef struct {

    uint8_t adv_addr[6];
    uint8_t init_addr[6];

} direct_packet_t;


typedef uint8_t ble_addr_t[6];

typedef enum {
    BLE_NO_ERROR,
    BLE_ERR_NOT_IDLE,
    BLE_ERR_NOLOCK,
    BLE_ERR_PDU_FULL,
} ble_status_t;

typedef enum {
    SCA_251_500,
    SCA_151_250,
    SCA_101_150,
    SCA_76_100,
    SCA_51_75,
    SCA_31_50,
    SCA_21_30,
    SCA_0_20

} scan_clock_accuracy_t;

typedef enum {

    BLE_IDLE,
    BLE_ADVERTISING,
    BLE_BEACONING,
    BLE_SCANNING,
    BLE_CONNECTED,

} ble_state_t;


typedef enum {
    PDU_ADV_IND,
    PDU_ADV_DIRECT_IND,
    PDU_ADV_NONCONN_IND,
    PDU_SCAN_REQ,
    PDU_SCAN_RSP,
    PDU_CONNECT_REQ,
    PDU_ADV_SCAN_IND,
} pdu_type_t;

typedef enum {
    HW_ADDR_TYPE_PUBLIC,
    HW_ADDR_TYPE_RANDOM
} ble_addr_type_t;


typedef union {
        uint8_t *buff;
} ble_payload_t;


/*
*   The main brain of the ble instance.  Since many radios have some form of dma that reequire
*       a specific memory layout to enable automating various things (encryption, crc, etc) then
*       the call to ble_radio_init will initialize the poitner to the payload buffer.  NOTE: this
*       is only the PDU payload and does not include the PDU header.
*/
typedef struct {
    thread_t        *ble_thread;
    void            *radio_handle;
    mutex_t         lock;
    ble_state_t     state;
    uint32_t        interval;
    uint32_t        crc_init;           // CRC initialization value for the link
    uint8_t         channel;
    uint32_t        access_address;
    uint8_t         hw_addr[6];
    ble_addr_type_t hw_addr_type;
    pdu_type_t      pdu_type;
    uint8_t         payload_length;
    ble_payload_t   payload;
} ble_t;




void ble_initialize(ble_t *ble_p);
void ble_set_sleepclock_accuracy(ble_t * instance_p, scan_clock_accuracy_t accuracy);
ble_status_t ble_pdu_add_shortname(ble_t *ble_p, uint8_t * str, uint8_t len);
ble_status_t ble_go_idle(ble_t *ble_p);

#define BLE_MAX_ADV_PDU_SIZE                64
#define BLE_MAX_DATA_PDU_SIZE               32


#define BLE_PREAMBLE_ADVERTISING            0Xaa
#define BLE_PREAMBLE_DATACHANNEL_LSB0       0x55
#define BLE_PREAMBLE_DATACHANNEL_LSB1       0xaa

#define BLE_ACCESSADDRESS_ADVERTISING       0x8e89bed6

#define PDU_HEADER_PDU_TYPE_Mask            0xf000
#define PDU_HEADER_PDU_TYPE_Pos             12
#define PDU_HEADER_TXADD_Mask               0x0200
#define PDU_HEADER_TXADD_Pos                9
#define PDU_HEADER_RXADD_Mask               0x0100
#define PDU_HEADER_RXADD_Pos                8
#define PDU_HEADER_LENGTH_Mask              0x00fc
#define PDU_HEADER_LENGTH_Pos               2

#define TXADD_PUBLIC                        0
#define TXADD_RANDOM                        1
#define RXADD_PUBLIC                        0
#define RXADD_RANDOM                        1

#define PDU_TYPE_ADV_IND                    0
#define PDU_TYPE_ADV_DIRECT_IND             1
#define PDU_TYPE_ADV_NONCONN_IND            2
#define PDU_TYPE_SCAN_REQ                   3
#define PDU_TYPE_SCAN_RESP                  4
#define PDU_TYPE_CONNECT_REQ                5
#define PDU_TYPE_ADV_SCAN_IND               6




#define BLE_CRC_POLYNOMIAL                  0x0100065b
#define BLE_CRC_INITIAL_ADV                 0x555555


#define GAP_ADTYPE_FLAGS                        0x01
#define GAP_ADTYPE_16BIT_MORE                   0x02
#define GAP_ADTYPE_16BIT_COMPLETE               0x03
#define GAP_ADTYPE_32BIT_MORE                   0x04
#define GAP_ADTYPE_32BIT_COMPLETE               0x05
#define GAP_ADTYPE_128BIT_MORE                  0x06
#define GAP_ADTYPE_128BIT_COMPLETE              0x07
#define GAP_ADTYPE_LOCAL_NAME_SHORT             0x08 //   Shortened local name
#define GAP_ADTYPE_LOCAL_NAME_COMPLETE          0x09 //   Complete local name
#define GAP_ADTYPE_POWER_LEVEL                  0x0A //   TX Power Level: 0xXX: -127 to +127 dBm
#define GAP_ADTYPE_OOB_CLASS_OF_DEVICE          0x0D
#define GAP_ADTYPE_OOB_SIMPLE_PAIRING_HASHC     0x0E
#define GAP_ADTYPE_OOB_SIMPLE_PAIRING_RANDR     0x0F
#define GAP_ADTYPE_SM_TK                        0x10
#define GAP_ADTYPE_SM_OOB_FLAG                  0x11
#define GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE    0x12
#define GAP_ADTYPE_SIGNED_DATA                  0x13
#define GAP_ADTYPE_SERVICES_LIST_16BIT          0x14
#define GAP_ADTYPE_SERVICES_LIST_128BIT         0x15
#define GAP_ADTYPE_SERVICE_DATA                 0x16
#define GAP_ADTYPE_APPEARANCE                   0x19
#define GAP_ADTYPE_MANUFACTURER_SPECIFIC        0xFF



#define BLE_CHECK_AND_LOCK(x)   \
        if (x->state != BLE_IDLE) \
            return BLE_ERR_NOT_IDLE; \
        if (mutex_acquire_timeout(&(x->lock),0) != NO_ERROR) \
            return BLE_ERR_NOLOCK;

#define BLE_UNLOCK(x)    \
        mutex_release(&(x->lock));


#endif