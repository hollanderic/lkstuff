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
#pragma once

#define VCORE_TAG_REQUEST 0x00000000
#define VCORE_ENDTAG	  0x00000000

#define VCORE_TAG_GET_FIRMWARE_REV			0x00000001
#define VCORE_TAG_GET_FIRMWARE_REV_REQ_LEN	0x00000000
#define VCORE_TAG_GET_FIRMWARE_REV_RSP_LEN	0x00000004

#define VCORE_MAILBOX_FULL	0x80000000
#define VCORE_MAILBOX_EMPTY	0x40000000

#define ARM_TO_VC_CHANNEL 0x08
#define VC_TO_ARM_CHANNEL 0x09

uint8_t * get_vcore_single(uint32_t tag, uint32_t req_len, uint32_t rsp_len);