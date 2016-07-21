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

#include <arch.h>
#include <arch/ops.h>
#include <arch/arm64.h>
#include <arch/arm64/mmu.h>
#include <platform/bcm28xx.h>
#include <platform/videocore.h>


/* Buffer for Videocore mailbox communications.  Needs to be 16 byte aligned
 *  but extending to 64 to ensure occupying full cache lines.
 */
volatile static uint8_t vc_mbox_buff[1024] __ALIGNED(64);
volatile static fb_mbox_t fb_mbox __ALIGNED(64);

uint32_t get_vcore_framebuffer(uint32_t * fb) {

	uint32_t i;

	

	fb_mbox.phys_width  = 640;
	fb_mbox.phys_height = 480;
	fb_mbox.virt_width  = 640;
	fb_mbox.virt_height = 480;
	fb_mbox.pitch		= 0;
	fb_mbox.depth		= 24;
	fb_mbox.virt_x_offs = 0;
	fb_mbox.virt_y_offs = 0;
	fb_mbox.fb_p		= 0;
	fb_mbox.fb_size		= 0;

	arch_clean_cache_range(&fb_mbox,sizeof(fb_mbox));

	if  (*REG32(ARM0_MAILBOX_STATUS) & VCORE_MAILBOX_FULL ) {
		printf("Mailbox full-ERR\n:");
		return 0;
	}
	uint32_t temp = ((uint32_t)&fb_mbox & 0x00000000fffffff0)  + VC_FB_CHANNEL + 0xC0000000;
	printf("temp= %08x\n",temp);
	ISB;
	DSB;
	*REG32(ARM0_MAILBOX_WRITE) = temp;
	ISB;
	DSB;
	i=0xffffffff;
	while (*REG32(ARM0_MAILBOX_STATUS) & VCORE_MAILBOX_EMPTY ) {
		i--;
		if (i==0) {
			printf("empty\n");
			return 0;
		}
	}

	uint32_t resp_addr;
	resp_addr = *REG32(ARM0_MAILBOX_READ);

	arch_invalidate_cache_range(&fb_mbox,sizeof(fb_mbox));

	*fb = fb_mbox.fb_p;
	return fb_mbox.fb_size;	

}



uint8_t * get_vcore_single(uint32_t tag, uint32_t req_len, uint32_t rsp_len) {
	int i;
	
	uint32_t * word_buff = vc_mbox_buff;
	word_buff[0] = 8 + sizeof(tag) + sizeof(req_len) + sizeof(rsp_len) + rsp_len + 1;
	word_buff[1] = VCORE_TAG_REQUEST;
	word_buff[2] = tag;
	word_buff[3] = req_len;
	word_buff[4] = rsp_len;
	for (i = 5; i< 5 + rsp_len; i++) {
		word_buff[i] = 0;
	}

	word_buff[i] = VCORE_ENDTAG;

	arch_clean_cache_range(vc_mbox_buff,sizeof(vc_mbox_buff));

	if  (*REG32(ARM0_MAILBOX_STATUS) & VCORE_MAILBOX_FULL ) {
		printf("Mailbox full-ERR\n:");
		return 0;
	}
	ISB;
	DSB;
	uint32_t temp;
	temp = (uint32_t)( (uint32_t)((uint32_t)word_buff & 0x00000000fffffff0)  + ARM_TO_VC_CHANNEL);
	//printf("buff at %llx\n",word_buff);
	//printf("writing %08x to location %llx\n",temp,ARM0_MAILBOX_WRITE);
	ISB;
	DSB;
	*REG32(ARM0_MAILBOX_WRITE) = temp;
	ISB;
	DSB;
	i=1000;
	while (*REG32(ARM0_MAILBOX_STATUS) & VCORE_MAILBOX_EMPTY ) {
		i--;
		if (i==0) {
			printf("empty\n");
			return 0;
		}
	}

	uint32_t * resp_addr;

	resp_addr = *REG32(ARM0_MAILBOX_READ);


	arch_invalidate_cache_range(vc_mbox_buff, sizeof(vc_mbox_buff));

	return &word_buff[5];

}





