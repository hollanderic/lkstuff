#include <stdio.h>
#include <err.h>
#include <kernel/timer.h>
#include <platform/spim.h>
#include <platform/nrf52.h>

#include <platform.h>
#include <dev/display.h>

#include <dev/gpio.h>
#include <target/gpioconfig.h>
#include "fpga_lib.h"
#include "pin_config.h"

#define BITBANG 1


//fbuf is the framebuffer which the lk gfx libraries write to (8-bit mono)
static uint16_t fbuf[64*32];

int fpga_spi_callback(int input);
static nrf_spim_dev_t spim0 = {
    .instance = NRF_SPIM0,
    .sclk_pin = SCK_PIN,
    .mosi_pin = MOSI_PIN,
    .miso_pin = 0x80000000,
    .speed = SPIM_SPEED_8M,
    .cb = fpga_spi_callback };



static void fpga_delay_us(uint32_t xms)
{
    lk_bigtime_t start = current_time_hires();
    while (start + xms > current_time_hires());
}


 void spi_send(uint8_t data) {
    for(int i = 0x80; i > 0; i=i/2){
        if (data & i)
            NRF_P0->OUTSET = 1 << MOSI_PIN;
        else
            NRF_P0->OUTCLR = 1 << MOSI_PIN;
        NRF_P0->OUTSET = 1 << SCK_PIN;
        NRF_P0->OUTCLR = 1 << SCK_PIN;
    }
}
static uint32_t bytes_remaining = 0;
static uint32_t xfers = 0;

int fpga_spi_callback(int input){
    if (bytes_remaining == 0) {
        gpio_set(CS_PIN, 0);
        //printf("sent %u transfers\n", xfers);
    } else {
        xfers++;
        uint8_t *b_ptr;
        uint8_t to_send;

        b_ptr = (uint8_t*)fbuf;
        b_ptr = &b_ptr[sizeof(fbuf) - bytes_remaining];
        to_send = (bytes_remaining > 255) ? 255 : bytes_remaining;
        bytes_remaining = bytes_remaining - to_send;
        nrf_spim_send(&spim0, b_ptr, to_send);
    }
    return 0;
}


void fpga_fbuf_refresh(void) {
    //fpga_test();
    xfers = 0;
    bytes_remaining = sizeof(fbuf);
    gpio_set(CS_PIN, 1);
    fpga_spi_callback(0);
}


static uint8_t color = 0;

void fpga_test(void)
{
    printf("Clearing display...\n");
    //uint8_t *temp = (uint8_t*)fbuf;
    uint16_t thiscolor = 0;

    for (int i=0; i<(64*32); i++){
        switch(i%2 + color%2) {
            case 0:
                thiscolor = 0xf800;
                break;
            case 1:
                thiscolor = 0x07e0;
                break;
            case 2:
                thiscolor = 0xffff;
                break;
        }
        fbuf[i] = thiscolor;
    }
    color++;
}


static timer_t disp_timer = TIMER_INITIAL_VALUE(disp_timer);

static enum handler_return disp_timer_cb(timer_t *timer, lk_time_t now, void *arg) {
    timer_set_oneshot(timer, 17, disp_timer_cb, NULL);
    fpga_fbuf_refresh();
    return 0;
}


void fpga_init() {
    gpio_config(CS_PIN, GPIO_OUTPUT);
    gpio_config(SCK_PIN, GPIO_OUTPUT);
    gpio_config(MOSI_PIN, GPIO_OUTPUT);
    gpio_set(CS_PIN, 0);
    gpio_set(SCK_PIN, 0);


    nrf_spim_init(&spim0);

    for (int i=0; i<(sizeof(fbuf)/2); i++)
        fbuf[i] = 0x1f00 ;
    disp_timer_cb(&disp_timer, 0, NULL);

    //fpga_fbuf_refresh();
}






#if 1
/* LK display (lib/gfx.h) calls this function */
status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    fb->image.pixels = (void *)fbuf;

    fb->format = DISPLAY_FORMAT_RGB_565;
    fb->image.format = IMAGE_FORMAT_RGB_565;
    fb->image.rowbytes = 128;

    fb->image.width = 64;
    fb->image.height = 32;
    fb->image.stride = 64;
    fb->flush = NULL;

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    info->format = DISPLAY_FORMAT_RGB_565;

    info->width = 64;
    info->height = 32;

    return NO_ERROR;
}

status_t display_present(struct display_image *image, uint starty, uint endy)
{
  //TRACEF("display_present - not implemented");
  //DEBUG_ASSERT(false);
  return NO_ERROR;
}

#endif
