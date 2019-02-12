#include <stdio.h>
#include <err.h>
#include <kernel/timer.h>
#include <platform/spim.h>
#include <platform/nrf52.h>

#include <platform.h>
#include <dev/display.h>

#include <dev/gpio.h>
#include <target/gpioconfig.h>
#include "ws_eink.h"
#include "pin_config.h"

#define BITBANG 1


//fbuf is the framebuffer which the lk gfx libraries write to (8-bit mono)
static uint8_t fbuf[EINK_WIDTH * EINK_HEIGHT];


const unsigned char lut_vcom0[] = {
    0x0E, 0x14, 0x01,
    0x0A, 0x06, 0x04,
    0x0A, 0x0A, 0x0F,
    0x03, 0x03, 0x0C,
    0x06, 0x0A, 0x00
};

const unsigned char lut_w[] = {
    0x0E, 0x14, 0x01,  //68                   34
    0x0A, 0x46, 0x04,  // 16 * 5 = 80         64
    0x8A, 0x4A, 0x0F,  // 20 * 16 = 320       300
    0x83, 0x43, 0x0C,  // 6 * 12 = 72         72
    0x86, 0x0A, 0x04   // 16 * 5 = 80         64
};

const unsigned char lut_b[] = {
    0x0E, 0x14, 0x01,
    0x8A, 0x06, 0x04,
    0x8A, 0x4A, 0x0F,
    0x83, 0x43, 0x0C,
    0x06, 0x4A, 0x04
};

const unsigned char lut_g1[] = {
    0x8E, 0x94, 0x01,
    0x8A, 0x06, 0x04,
    0x8A, 0x4A, 0x0F,
    0x83, 0x43, 0x0C,
    0x06, 0x0A, 0x04
};

const unsigned char lut_g2[] = {
    0x8E, 0x94, 0x01,
    0x8A, 0x06, 0x04,
    0x8A, 0x4A, 0x0F,
    0x83, 0x43, 0x0C,
    0x06, 0x0A, 0x04
};

const unsigned char lut_vcom1[] = {
    0x03, 0x1D, 0x01,
    0x01, 0x08, 0x23,
    0x37, 0x37, 0x01,
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00
};

const unsigned char lut_red0[] = {
    0x83, 0x5D, 0x01, // 16 * 2 = 32  16
    0x81, 0x48, 0x23, // 9 * 36 = 324  315
    0x77, 0x77, 0x01, // 110 * 2 = 220 110
    0x00, 0x00, 0x00,
    0x00, 0x00, 0x00
};

const unsigned char lut_red1[] = {
    0x03, 0x1D, 0x01, 0x01, 0x08, 0x23, 0x37, 0x37,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


//extern const unsigned char IMAGE_BLACK[];
//extern const unsigned char IMAGE_RED[];

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



static void ws_eink_delay_ms(uint32_t xms)
{
    lk_bigtime_t start = current_time_hires();
    while (start + xms*1000 > current_time_hires());
}

static void ws_eink_wait_idle(void)
{
    lk_bigtime_t start = current_time_hires();

    while(1) {      //LOW: busy, HIGH: idle
        if(gpio_get(BUSY_PIN))
            break;
    }
    printf("waited %llu\n", current_time_hires() - start);
    ws_eink_delay_ms(200);

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

static void ws_eink_reset(void) {
    gpio_set(RST_PIN, 1);
    ws_eink_delay_ms(100);
    gpio_set(RST_PIN, 0);
    ws_eink_delay_ms(100);
    gpio_set(RST_PIN, 1);
    ws_eink_delay_ms(100);
}

static void ws_eink_write_command(uint8_t command) {
    gpio_set(DC_PIN, 0);
    gpio_set(CS_PIN, 0);
    spi_send(command);
    gpio_set(CS_PIN, 1);
}

static void ws_eink_write_data(uint8_t command) {
    gpio_set(DC_PIN, 1);
    gpio_set(CS_PIN, 0);
    spi_send(command);
    gpio_set(CS_PIN, 1);
}

static void ws_eink_set_bw_lut(void)
{
    uint32_t count;
    ws_eink_write_command(0x20);         //g vcom
    for(count = 0; count < 15; count++) {
        ws_eink_write_data(lut_vcom0[count]);
    }
    ws_eink_write_command(0x21);        //g ww --
    for(count = 0; count < 15; count++) {
        ws_eink_write_data(lut_w[count]);
    }
    ws_eink_write_command(0x22);         //g bw r
    for(count = 0; count < 15; count++) {
        ws_eink_write_data(lut_b[count]);
    }
    ws_eink_write_command(0x23);         //g wb w
    for(count = 0; count < 15; count++) {
        ws_eink_write_data(lut_g1[count]);
    }
    ws_eink_write_command(0x24);         //g bb b
    for(count = 0; count < 15; count++) {
        ws_eink_write_data(lut_g2[count]);
    }
}


static void ws_eink_set_red_lut(void)
{
    uint32_t count;
    ws_eink_write_command(0x25);
    for(count = 0; count < 15; count++) {
        ws_eink_write_data(lut_vcom1[count]);
    }
    ws_eink_write_command(0x26);
    for(count = 0; count < 15; count++) {
        ws_eink_write_data(lut_red0[count]);
    }
    ws_eink_write_command(0x27);
    for(count = 0; count < 15; count++) {
        ws_eink_write_data(lut_red1[count]);
    }
}


uint8_t ws_eink_config(void)
{
    ws_eink_reset();

    ws_eink_write_command(POWER_SETTING);  //0x01
    ws_eink_write_data(0x07);
    ws_eink_write_data(0x00);
    ws_eink_write_data(0x08);
    ws_eink_write_data(0x00);
    ws_eink_write_command(BOOSTER_SOFT_START); //0x06
    ws_eink_write_data(0x07);
    ws_eink_write_data(0x07);
    ws_eink_write_data(0x07);
    ws_eink_write_command(POWER_ON); //0x04

    ws_eink_wait_idle();

    ws_eink_write_command(PANEL_SETTING); //0x00
    ws_eink_write_data(0xcf);
    ws_eink_write_command(VCOM_AND_DATA_INTERVAL_SETTING); //0x50
    ws_eink_write_data(0xF0);
    ws_eink_write_command(PLL_CONTROL);
    ws_eink_write_data(0x39);
    ws_eink_write_command(TCON_RESOLUTION);  //set x and y
    ws_eink_write_data(0xC8);            //x
    ws_eink_write_data(0x00);            //y High eight
    ws_eink_write_data(0xC8);            //y Low eight
    ws_eink_write_command(VCM_DC_SETTING_REGISTER); //VCOM
    ws_eink_write_data(0x0E);

    ws_eink_set_bw_lut();
    ws_eink_set_red_lut();

    return 0;
}


void ws_eink_clear(void)
{
    printf("Clearing display...\n");
    uint32_t width, height;
    width = (EINK_WIDTH % 8 == 0)? (EINK_WIDTH / 8 ): (EINK_WIDTH / 8 + 1);
    height = EINK_HEIGHT;

    //send black data
    ws_eink_write_command(DATA_START_TRANSMISSION_1);
    ws_eink_delay_ms(2);
    for(uint32_t i = 0; i < height; i++) {
        for(uint32_t i = 0; i < width; i++) {
            ws_eink_write_data(0xFF);
            ws_eink_write_data(0xFF);
        }
    }
    ws_eink_delay_ms(2);

    //send red data
    ws_eink_write_command(DATA_START_TRANSMISSION_2);
    ws_eink_delay_ms(2);
    for(uint32_t i = 0; i < height; i++) {
        for(uint32_t i = 0; i < width; i++) {
            ws_eink_write_data(0xFF);
        }
    }
    ws_eink_delay_ms(2);

    ws_eink_write_command(DISPLAY_REFRESH);
    ws_eink_wait_idle();
    printf("Cleared!\n");

}

/******************************************************************************
function :  Sends the image buffer in RAM to e-Paper and displays
parameter:
******************************************************************************/
void ws_eink_display(const uint8_t *blackimage, const uint8_t *redimage)
{
    uint8_t temp = 0x00;
    uint32_t width, height;
    width = (EINK_WIDTH % 8 == 0)? (EINK_WIDTH / 8 ): (EINK_WIDTH / 8 + 1);
    height = EINK_HEIGHT;

    ws_eink_write_command(DATA_START_TRANSMISSION_1);
    for (uint32_t j = 0; j < height; j++) {
        for (uint32_t i = 0; i < width; i++) {
            temp = 0x00;
            for (int bit = 0; bit < 4; bit++) {
                if ((blackimage[i + j * width] & (0x80 >> bit)) != 0) {
                    temp |= 0xC0 >> (bit * 2);
                }
            }
            ws_eink_write_data(temp);
            temp = 0x00;
            for (int bit = 4; bit < 8; bit++) {
                if ((blackimage[i + j * width] & (0x80 >> bit)) != 0) {
                    temp |= 0xC0 >> ((bit - 4) * 2);
                }
            }
            ws_eink_write_data(temp);
        }
    }
    ws_eink_delay_ms(2);

    ws_eink_write_command(DATA_START_TRANSMISSION_2);
    for (uint32_t j = 0; j < height; j++) {
        for (uint32_t i = 0; i < width; i++) {
            ws_eink_write_data(redimage[i + j * width]);
        }
    }
    ws_eink_delay_ms(2);

    //Display refresh
    ws_eink_write_command(DISPLAY_REFRESH);
    ws_eink_wait_idle();

}



void ws_eink_init() {
    gpio_config(CS_PIN, GPIO_OUTPUT);
    gpio_config(DC_PIN, GPIO_OUTPUT);
    gpio_config(RST_PIN, GPIO_OUTPUT);
    gpio_config(SCK_PIN, GPIO_OUTPUT);
    gpio_config(MOSI_PIN, GPIO_OUTPUT);
    gpio_config(BUSY_PIN, GPIO_OUTPUT);

    gpio_set(CS_PIN, 1);
    gpio_set(SCK_PIN, 0);

#if !BITBANG
    nrf_spim_init(&spim0);
#endif
    ws_eink_config();
    ws_eink_clear();
}


/* LK display (lib/gfx.h) calls this function */
status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    fb->image.pixels = (void *)fbuf;

    fb->format = DISPLAY_FORMAT_MONO_1;
    fb->image.format = IMAGE_FORMAT_MONO_8;
    fb->image.rowbytes = EINK_WIDTH;

    fb->image.width = EINK_WIDTH;
    fb->image.height = EINK_HEIGHT;
    fb->image.stride = EINK_WIDTH;
    fb->flush = NULL;

    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    info->format = DISPLAY_FORMAT_MONO_1;

    info->width = EINK_WIDTH;
    info->height = EINK_HEIGHT;

    return NO_ERROR;
}

status_t display_present(struct display_image *image, uint starty, uint endy)
{
  //TRACEF("display_present - not implemented");
  //DEBUG_ASSERT(false);
  return NO_ERROR;
}
