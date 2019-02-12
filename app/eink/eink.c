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

#include "ws_eink.h"
#include "image.c"

//static uint8_t fbuf[COLUMNS * ROWS / 8];


static uint8_t channel=0;


void draw(void);
void clear(void);

STATIC_COMMAND_START
STATIC_COMMAND("display", "Draw e-ink display", (console_cmd)&draw)
STATIC_COMMAND("clear", "Clear e-ink display", (console_cmd)&clear)

STATIC_COMMAND_END(eink);



static void delay(uint32_t d) {
    lk_bigtime_t start = current_time_hires();
    while (start + d > current_time_hires());
}



static thread_t *tester;




void draw(void){
    dprintf(SPEW,"updated %u\n", current_time());
    ws_eink_display(IMAGE_BLACK, IMAGE_RED);

}

void clear(void){
    lk_bigtime_t x = current_time_hires();
    dprintf(SPEW, "%llu usec\n", x);
    ws_eink_clear();
}


static void eink_init(const struct app_descriptor *app)
{
    ws_eink_init();

}

static void eink_entry(const struct app_descriptor *app, void *args)
{

}

APP_START(eink)
.init = eink_init,
.entry = eink_entry,
APP_END

