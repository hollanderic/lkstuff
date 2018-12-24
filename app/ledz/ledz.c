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
#include <platform/spim.h>

#include <platform.h>



#define LED_CLK 15
#define LED_DATA 16


#define LED_A 22
#define LED_B 23
#define LED_C 24
#define LED_D 25

#define LED_OE 21
#define LED_STRB 17

//For this to work, daisy chain all six segments together
#define COLUMNS (64)
#define ROWS (32)

//pixbuf is used to shift data out to the display in the format
// needed by the displays configuration
static uint8_t pixbuf[3 * COLUMNS * ROWS / 8];

//Actual framebuffer in 332 format
static uint8_t fbuf[COLUMNS * ROWS];

static uint8_t channel=0;

void shift(void);
void curr_t(void);
void hcurr_t(void);

STATIC_COMMAND_START
STATIC_COMMAND("shift", "shift a line", (console_cmd)&shift)
STATIC_COMMAND("time", "get current_time", (console_cmd)&curr_t)
STATIC_COMMAND("htime", "get current_time_high_res", (console_cmd)&hcurr_t)

STATIC_COMMAND_END(ledz);
int spi_callback(int input);
static nrf_spim_dev_t spim0 = {
    .instance = NRF_SPIM0,
    .sclk_pin = LED_CLK,
    .mosi_pin = LED_DATA,
    .miso_pin = 0x80000000,
    .speed = SPIM_SPEED_8M,
    .cb = spi_callback };


static inline uint32_t bitindex(uint32_t x, uint32_t y, uint32_t color) {
    return (2 * 3 * COLUMNS - 1) - (((y >> 4) * COLUMNS) + (63 - x) + (2 * COLUMNS * color));
}

// copies the fbuf over to pixbuf
static void convert_buf(void) {
    for (uint32_t i = 0; i < sizeof(pixbuf); i++)
        pixbuf[i] = 0;
    for (uint32_t y = 0; y < ROWS; y++){
        for(uint32_t x = 0; x < COLUMNS; x++) {
            uint32_t pixrow = ((3 * 2 * COLUMNS) >> 3) * (y & 0x0f);
            uint32_t bitpos = bitindex(x, y, 0);
            pixbuf[pixrow + (bitpos >> 3)] |= ((fbuf[COLUMNS * y + x] & 0xe0) ? 1 : 0) << (bitpos % 8);
            bitpos = bitindex(x, y, 1);
            pixbuf[pixrow + (bitpos >> 3)] |= ((fbuf[COLUMNS * y + x] & 0x1c) ? 1 : 0) << (bitpos % 8);
            bitpos = bitindex(x, y, 2);
            pixbuf[pixrow + (bitpos >> 3)] |= ((fbuf[COLUMNS * y + x] & 0x03) ? 1 : 0) << (bitpos % 8);
        }
    }
}

static void delay(uint32_t d) {
    lk_bigtime_t start = current_time_hires();
    while (start + d > current_time_hires());
}

int spi_callback(int input) {
    channel++;
    gpio_set(LED_OE, 1);
    gpio_set(LED_STRB, 1);
    gpio_set(LED_A, (channel & 0x01) ? 1 : 0);
    gpio_set(LED_B, (channel & 0x02) ? 1 : 0);
    gpio_set(LED_C, (channel & 0x04) ? 1 : 0);
    gpio_set(LED_D, (channel & 0x08) ? 1 : 0);

    gpio_set(LED_STRB, 0);
    delay(100);
    gpio_set(LED_OE, 0);
    nrf_spim_send(&spim0, &pixbuf[((channel+1) & 0x0f) * 48], 48);
    return 0;
}

void curr_t(void){
    dprintf(SPEW,"%u\n", current_time());
}

void hcurr_t(void){
    lk_bigtime_t x = current_time_hires();
    dprintf(SPEW, "%llu usec\n", x);
}

void shift(void)
{
        convert_buf();
        //nrf_spim_send(&spim0, pixbuf, COLUMNS / 8);
}


static void ledz_init(const struct app_descriptor *app)
{
    gpio_config(LED_CLK, GPIO_OUTPUT);
    gpio_config(LED_DATA, GPIO_OUTPUT);
    gpio_config(LED_A, GPIO_OUTPUT);
    gpio_config(LED_B, GPIO_OUTPUT);
    gpio_config(LED_C, GPIO_OUTPUT);
    gpio_config(LED_D, GPIO_OUTPUT);
    gpio_config(LED_OE, GPIO_OUTPUT);
    gpio_config(LED_STRB, GPIO_OUTPUT);

    gpio_set(LED_CLK, 0);
    gpio_set(LED_STRB, 0);

    for(uint32_t i = 0; i < sizeof(pixbuf); i++) {
        pixbuf[i] = 0xff;
    }

    for(uint32_t y = 0; y < ROWS; y++)
        for(uint32_t x = 0; x < COLUMNS; x++) {
            switch (x % 3) {
                case 0:
                    fbuf[y * COLUMNS + x] = 0xe0;
                    break;
                case 1:
                    fbuf[y * COLUMNS + x] = 0x1c;
                    break;
                case 2:
                    fbuf[y * COLUMNS + x] = 0x03;
                    break;
            }
        }

    nrf_spim_init(&spim0);

    nrf_spim_send(&spim0, pixbuf, 8);

}

static void ledz_entry(const struct app_descriptor *app, void *args)
{

}

APP_START(ledz)
.init = ledz_init,
.entry = ledz_entry,
APP_END

