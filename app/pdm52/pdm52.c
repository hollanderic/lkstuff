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

#define PDM_BUFF_SIZE 4096

#define PDM_CLK_PIN   15
#define PDM_DATA_PIN   16

static uint32_t buff1[PDM_BUFF_SIZE];
static uint32_t buff2[PDM_BUFF_SIZE];
static uint32_t*  last_buff;

static void pdm52_init(const struct app_descriptor *app)
{
    printf("Initializing Pulse Density Modulation nrf52 driver...\n");
    gpio_config(PDM_CLK_PIN,GPIO_OUTPUT);
    gpio_config(PDM_DATA_PIN,GPIO_INPUT);

    NRF_PDM->PSEL.CLK = PDM_CLK_PIN;
    NRF_PDM->PSEL.DIN = PDM_DATA_PIN;
    NRF_PDM->ENABLE = 1;

    NRF_PDM->MODE = 0x00000003;

    NRF_PDM->SAMPLE.PTR = (uint32_t)buff1;
    last_buff = buff1;
    NRF_PDM->SAMPLE.MAXCNT = PDM_BUFF_SIZE;
    NRF_PDM->EVENTS_STARTED = 0;
    NRF_PDM->INTEN = 0x01;


    NVIC_ClearPendingIRQ(PDM_IRQn);
    NVIC_EnableIRQ(PDM_IRQn);

    NRF_PDM->TASKS_START = 1;

/*
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
*/
}

static void pdm52_entry(const struct app_descriptor *app, void *args)
{
    printf("hitting Pulse Density Modulation nrf52 entry...\n");
}

static void dump(uint8_t* buff) {
    printf("%02x %02x %02x %02x\n",buff[0],buff[1],buff[2],buff[3]);
}

static inline int16_t abs(int16_t input) {
    return (input < 0) ? -1*input : input;
}

int32_t buff_average(uint32_t* buff) {
    int32_t avg = 0;
    for (int i = 0; i < PDM_BUFF_SIZE; i++) {
        avg = avg + abs((int16_t)(buff[i] & 0xffff));
    }
    return avg >> 12;
}

void nrf52_PDM_IRQ (void) {

    arm_cm_irq_entry();
    int32_t average;

    NRF_PDM->EVENTS_STARTED = 0;
    if (last_buff == buff1) {
        average = buff_average(buff2);
        last_buff = buff2;
        NRF_PDM->SAMPLE.PTR = (uint32_t)buff2;
    } else {
        average = buff_average(buff1);
        last_buff = buff1;
        NRF_PDM->SAMPLE.PTR = (uint32_t)buff1;
    }
    NRF_PDM->SAMPLE.MAXCNT = PDM_BUFF_SIZE;

    printf("%d\n",average);
    arm_cm_irq_exit(true);
}

#if defined(WITH_LIB_CONSOLE)
#include <lib/console.h>


static int dump_buffer_cmd(int argc, const cmd_args *argv)
{



    return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("pdmdump", "dump pdm buffer", (console_cmd)&dump_buffer_cmd)
STATIC_COMMAND_END(pdm52);

#endif

APP_START(pdm52)
    .flags = 0,
    .init = pdm52_init,
    .entry = pdm52_entry,
 APP_END

