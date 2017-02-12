/*
 * Copyright (c) 2017 Eric Holland
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
#include <stdio.h>
#include <compiler.h>
#include <arch/arm/cm.h>
#include <dev/gpio.h>
#include <platform/gpio.h>
#include <platform/nrf52.h>

#define NUM_PIXELS 60

#define NEOPIXEL_SCLK_PIN   11
#define NEOPIXEL_MOSI_PIN   12
#define NEOPIXEL_MISO_PIN   13


/* In order to use a SPI, we use 4 bits to encode one bit
        logic1 = b1100
        logic0 = b1000
    this also means that one encoded byte becomes four bytes
*/
typedef struct {
    uint8_t red[4];
    uint8_t green[4];
    uint8_t blue[4];
} neopixel_t;


typedef struct {
    uint8_t start[15];
    neopixel_t data[NUM_PIXELS];
    uint8_t stop[40];       //Serves as the stop sync
} pixel_array_t;

static pixel_array_t pixel_array;

static neopixel_t* pixel_data= pixel_array.data;



static void encode(uint8_t in,uint8_t* buff) {
    uint8_t val = in;

    for (int i=0; i < 4; i++) {

        uint8_t temp = (( val & (1 << (7 - 2*i))) ? 0xe0 : 0x80) |
                  (( val & (1 << (6 - 2*i))) ? 0x0e : 0x08);
        buff[i] = temp;
    }
}


static void write_pixel(uint32_t pix_id, uint8_t red, uint8_t green, uint8_t blue) {

    encode( red, pixel_data[pix_id].red);
    encode( green, pixel_data[pix_id].green);
    encode( blue, pixel_data[pix_id].blue);


}




static void neopixel52_init(const struct app_descriptor *app)
{
    printf("Initializing NeoPixel nrf52 driver...\n");
    printf("Configuring SPI Master...\n");
    gpio_config(NEOPIXEL_SCLK_PIN,GPIO_OUTPUT);
    gpio_config(NEOPIXEL_MOSI_PIN,GPIO_OUTPUT);
    gpio_config(NEOPIXEL_MISO_PIN,GPIO_INPUT | GPIO_PULLUP);


    NRF_SPIM0->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M4; //1MHz

    NRF_SPIM0->PSEL.SCK = NEOPIXEL_SCLK_PIN;
    NRF_SPIM0->PSEL.MOSI = NEOPIXEL_MOSI_PIN;
    NRF_SPIM0->PSEL.MISO = NEOPIXEL_MISO_PIN;

    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Enabled;

    NRF_SPIM0->EVENTS_ENDTX = 0;
    NRF_SPIM0->INTENSET = SPIM_INTENSET_ENDTX_Set << SPIM_INTENSET_ENDTX_Pos;
    NVIC_ClearPendingIRQ(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);
    NVIC_EnableIRQ(SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn);

    uint8_t* temp = &pixel_array;
    for (int i=0;  i<sizeof(pixel_array_t); i++)
        temp[i]=0;
}

static void neopixel52_entry(const struct app_descriptor *app, void *args)
{
    printf("hitting NeoPixel nef52 entry...\n");
}

static void dump(uint8_t* buff) {
    printf("%02x %02x %02x %02x\n",buff[0],buff[1],buff[2],buff[3]);
}


static uint32_t total_bytes=0;
static uint32_t sent_bytes=0;
static uint32_t next_pos=0;

static void start_load(void) {

    total_bytes = sizeof(pixel_array_t);
    sent_bytes =(total_bytes > 255) ? 255 : total_bytes;
    next_pos = sent_bytes;
    printf("sending %d total bytes\n",total_bytes);
    printf(" starting at %x\n",&pixel_array);

    NRF_SPIM0->EVENTS_ENDTX = 0;
    NRF_SPIM0->INTENSET = SPIM_INTENSET_ENDTX_Set << SPIM_INTENSET_ENDTX_Pos;
    NRF_SPIM0->TXD.PTR = &pixel_array;
    NRF_SPIM0->RXD.PTR = 0;
    NRF_SPIM0->TXD.MAXCNT = sent_bytes;
    NRF_SPIM0->RXD.MAXCNT = 0;

    NRF_SPIM0->TXD.LIST = 1;
    NRF_SPIM0->RXD.LIST = 1;
    NRF_SPIM0->TASKS_START = 1;
}



void nrf52_SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQ (void) {

    arm_cm_irq_entry();

    uint32_t remaining = total_bytes - sent_bytes;
    NRF_SPIM0->EVENTS_ENDTX = 0;

    if (remaining <= 0 ) {

        NRF_SPIM0->INTENCLR = SPIM_INTENCLR_ENDTX_Clear << SPIM_INTENCLR_ENDTX_Pos;
    } else {
        //printf("restarting at %x, %d\n",(uint32_t)&pixel_array + next_pos,next_pos);
        NRF_SPIM0->TXD.PTR = (((uint32_t)&pixel_array) + next_pos);
        uint32_t count = (remaining > 255)? 255 : remaining;
        NRF_SPIM0->TXD.MAXCNT = count;
        NRF_SPIM0->TASKS_START = 1;
        next_pos = next_pos + count;
        sent_bytes = sent_bytes + count;
    }

    //printf("Radio IRQ fired\n");
    arm_cm_irq_exit(true);
}

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>


static int write_pixel_cmd(int argc, const cmd_args *argv)
{

    for (int i=0; i<NUM_PIXELS; i++ )
        write_pixel(i,argv[1].i, argv[2].i, argv[3].i);

    start_load();

    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("neowrite", "write first pixel", (console_cmd)&write_pixel_cmd)
STATIC_COMMAND_END(neopixel52);

#endif

APP_START(neopixel52)
    .flags = 0,
    .init = neopixel52_init,
    .entry = neopixel52_entry,
 APP_END

