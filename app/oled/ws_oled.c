#include <stdio.h>
#include <err.h>
#include <kernel/timer.h>
#include <platform/spim.h>
#include <platform/nrf52.h>

#include <platform.h>
#include <dev/display.h>

#include <dev/gpio.h>
#include <target/gpioconfig.h>
#include "ws_oled.h"
#include "pin_config.h"

#define BITBANG 1


//fbuf is the framebuffer which the lk gfx libraries write to (8-bit mono)
static uint8_t fbuf[OLED_WIDTH * OLED_HEIGHT];



#if !BITBANG
static nrf_spim_dev_t spim0 = {
    .instance = NRF_SPIM0,
    .sclk_pin = SCK_PIN,
    .mosi_pin = MOSI_PIN,
    .miso_pin = 0x80000000,
    .speed = SPIM_SPEED_8M,
    .cb = NULL
};
#endif

static void ws_oled_delay_ms(uint32_t xms)
{
    lk_bigtime_t start = current_time_hires();
    while (start + xms*1000 > current_time_hires());
}

static inline void spi_send(uint8_t data) {
    NRF_P0->OUTCLR = 1 << CS_PIN;
#if BITBANG
    for(int i = 0x80; i > 0; i=i/2){
        if (data & i)
            NRF_P0->OUTSET = 1 << MOSI_PIN;
        else
            NRF_P0->OUTCLR = 1 << MOSI_PIN;
        NRF_P0->OUTSET = 1 << SCK_PIN;
        NRF_P0->OUTSET = 1 << SCK_PIN;
        NRF_P0->OUTCLR = 1 << SCK_PIN;
    }
#else
    nrf_spim_send(&spim0, &data, 1);
#endif
    NRF_P0->OUTSET = 1 << CS_PIN;
}

static void ws_oled_reset(void) {
    gpio_set(RST_PIN, 1);
    ws_oled_delay_ms(100);
    gpio_set(RST_PIN, 0);
    ws_oled_delay_ms(100);
    gpio_set(RST_PIN, 1);
    ws_oled_delay_ms(100);
}

static void ws_oled_write_command(uint8_t command) {
    gpio_set(DC_PIN, 0);
    gpio_set(CS_PIN, 0);
    spi_send(command);
    gpio_set(CS_PIN, 1);
}

static void ws_oled_setwindow(void) {
    ws_oled_write_command(0x15);    //   set column address
    ws_oled_write_command(0x00);    //  start column   0
    ws_oled_write_command(0x7f);    //  end column   127

    ws_oled_write_command(0x75);    //   set row address
    ws_oled_write_command(0x00);    //  start row   0
    ws_oled_write_command(0x7f);    //  end row   127
}

static void ws_oled_config(void) {

    ws_oled_reset();

    ws_oled_write_command(0xae);//--turn off oled panel

    ws_oled_setwindow();

    ws_oled_write_command(0x81);  // set contrast control
    ws_oled_write_command(0x80);

    ws_oled_write_command(0xa0);    // gment remap
    ws_oled_write_command(0x51);   //51

    ws_oled_write_command(0xa1);  // start line
    ws_oled_write_command(0x00);

    ws_oled_write_command(0xa2);  // display offset
    ws_oled_write_command(0x00);

    ws_oled_write_command(0xa4);    // rmal display
    ws_oled_write_command(0xa8);    // set multiplex ratio
    ws_oled_write_command(0x7f);

    ws_oled_write_command(0xb1);  // set phase leghth
    ws_oled_write_command(0xf1);

    ws_oled_write_command(0xb3);  // set dclk
    ws_oled_write_command(0xf0);  //80Hz:0xc1 90Hz:0xe1   100Hz:0x00   110Hz:0x30 120Hz:0x50   130Hz:0x70     01

    ws_oled_write_command(0xab);  //
    ws_oled_write_command(0x01);  //

    ws_oled_write_command(0xb6);  // set phase leghth
    ws_oled_write_command(0x0f);

    ws_oled_write_command(0xbe);
    ws_oled_write_command(0x0f);

    ws_oled_write_command(0xbc);
    ws_oled_write_command(0x08);

    ws_oled_write_command(0xd5);
    ws_oled_write_command(0x62);

    ws_oled_write_command(0xfd);
    ws_oled_write_command(0x12);

    ws_oled_delay_ms(200);

    ws_oled_write_command(0xaf);    //turn on display
}

void ws_oled_display() {
    ws_oled_setwindow();
    gpio_set(DC_PIN, 1);
    for (int i = 0; i < sizeof(fbuf)/2; i++) {
        spi_send((fbuf[2*i] & 0xf0) | (fbuf[2*i + 1] >> 4));
    }
}

static timer_t disp_timer = TIMER_INITIAL_VALUE(disp_timer);

static enum handler_return disp_timer_cb(timer_t *timer, lk_time_t now, void *arg) {
    timer_set_oneshot(timer, 50, disp_timer_cb, NULL);
    ws_oled_display();
    return 0;
}

void ws_oled_init() {
    gpio_config(CS_PIN, GPIO_OUTPUT);
    gpio_config(DC_PIN, GPIO_OUTPUT);
    gpio_config(RST_PIN, GPIO_OUTPUT);
    gpio_config(SCK_PIN, GPIO_OUTPUT);
    gpio_config(MOSI_PIN, GPIO_OUTPUT);
    gpio_set(CS_PIN, 1);
    gpio_set(SCK_PIN, 0);

#if !BITBANG
    nrf_spim_init(&spim0);
#endif
    ws_oled_config();

    for (uint32_t i = 0; i < sizeof(fbuf); i++)
        fbuf[i] = (i % 16) << 4;
    //ws_oled_display();
    disp_timer_cb(&disp_timer, 0, NULL);
}


/* LK display (lib/gfx.h) calls this function */
status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    fb->image.pixels = (void *)fbuf;

    fb->format = DISPLAY_FORMAT_MONO_1;
    fb->image.format = IMAGE_FORMAT_MONO_8;
    fb->image.rowbytes = OLED_WIDTH;

    fb->image.width = OLED_WIDTH;
    fb->image.height = OLED_HEIGHT;
    fb->image.stride = OLED_WIDTH;
    fb->flush = NULL;

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    info->format = DISPLAY_FORMAT_MONO_1;

    info->width = OLED_WIDTH;
    info->height = OLED_HEIGHT;

    return NO_ERROR;
}

status_t display_present(struct display_image *image, uint starty, uint endy)
{
  //TRACEF("display_present - not implemented");
  //DEBUG_ASSERT(false);
  return NO_ERROR;
}
