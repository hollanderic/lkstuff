/*
 * Copyright (c) 2009 Travis Geiselbrecht
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
#include <app.h>
#include <debug.h>
#include <kernel/thread.h>
#include <lib/console.h>


#include <platform.h>

#include "ws_oled.h"

//static uint8_t fbuf[COLUMNS * ROWS / 8];


static uint8_t channel=0;


void curr_t(void);
void hcurr_t(void);

STATIC_COMMAND_START
STATIC_COMMAND("display", "run oled_display", (console_cmd)&curr_t)
STATIC_COMMAND("htime", "get current_time_high_res", (console_cmd)&hcurr_t)

STATIC_COMMAND_END(oled);



static void delay(uint32_t d) {
    lk_bigtime_t start = current_time_hires();
    while (start + d > current_time_hires());
}



static thread_t *tester;




void curr_t(void){
    dprintf(SPEW,"updated %u\n", current_time());
    ws_oled_display();

}

void hcurr_t(void){
    lk_bigtime_t x = current_time_hires();
    dprintf(SPEW, "%llu usec\n", x);
}


static void oled_init(const struct app_descriptor *app)
{
    ws_oled_init();

}

static void oled_entry(const struct app_descriptor *app, void *args)
{

}

APP_START(oled)
.init = oled_init,
.entry = oled_entry,
APP_END
