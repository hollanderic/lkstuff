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

#ifndef __BLE_RADIO_H
#define __BLE_RADIO_H

#include <lib/ble.h>

/*
    Initialize the radio to ble mode, place in idle.
    This will perform necessary checks to ensure configuration
    is viable to ramp up for transmission or reception.
*/
void ble_radio_initialize(ble_t *ble_p);

/*
 *	Returns hw addr(mac) and type.  platforms not suporting feature return -1
 *
 */
int32_t ble_get_hw_addr(ble_t *ble_p);

uint32_t ble_radio_shutdown(void);

int32_t ble_radio_set_channel(ble_t * ble_p, uint8_t channel);



uint32_t ble_radio_tx(ble_t * ble_p);



 #endif