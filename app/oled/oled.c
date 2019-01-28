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
#include <dev/gpio.h>
#include <target/gpioconfig.h>

#include <platform.h>

#include "DEV_Config.h"
#include "OLED_Driver.h"

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
    dprintf(SPEW,"%u\n", current_time());
    OLED_Display();
}

void hcurr_t(void){
    lk_bigtime_t x = current_time_hires();
    dprintf(SPEW, "%llu usec\n", x);
}


static void oled_init(const struct app_descriptor *app)
{
    gpio_config(CS_PIN, GPIO_OUTPUT);
    gpio_config(DC_PIN, GPIO_OUTPUT);
    gpio_config(RST_PIN, GPIO_OUTPUT);
    gpio_config(SCK_PIN, GPIO_OUTPUT);
    gpio_config(MOSI_PIN, GPIO_OUTPUT);
#if 0
    gpio_config(LED_D, GPIO_OUTPUT);
    gpio_config(LED_OE, GPIO_OUTPUT);
    gpio_config(LED_STRB, GPIO_OUTPUT);

    gpio_set(LED_A, 1);
    gpio_set(LED_B, 1);
    gpio_set(LED_C, 0);
    gpio_set(LED_D, 0);
#endif
    System_Init();
    OLED_Init(0);
    OLED_Clear(0xffff);
    OLED_Display();
}

static void oled_entry(const struct app_descriptor *app, void *args)
{

}

APP_START(oled)
.init = oled_init,
.entry = oled_entry,
APP_END

