#include <dev/display.h>
#include <dev/et011tt2v1.h>

static struct display_framebuffer eink_framebuffer;

static eink_t eink_display;
static uint8_t eink_fb[FBSIZE];

static SPI_HandleTypeDef SpiHandle;


static uint8_t clamp_px(uint8_t px)
{
    if (px) return 0x3;
    return 0x0;
}


int display_init(void *fb){


    SpiHandle.Instance               = SPI2;
    SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    SpiHandle.Init.Direction         = SPI_DIRECTION_1LINE;
    SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
    SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
    SpiHandle.Init.DataSize          = SPI_DATASIZE_9BIT;
    SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
    SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    SpiHandle.Init.CRCPolynomial     = 0;
    SpiHandle.Init.NSS               = SPI_NSS_SOFT;
    SpiHandle.Init.Mode              = SPI_MODE_MASTER;

    err = spi_init(&SpiHandle);
    if (err != HAL_OK) {
        printf("Failed to init spi\n");
    }

    eink_display.spi_handle = (uint32_t) &SpiHandle;
    eink_display.with_busy = false;
    eink.display.framebuff = eink_fb;
    eink_display.cs_gpio   =
    eink_display.rst_gpio  =
    eink_display.busy_gpio = 

	eink_init(&eink_display);






}


status_t display_present(struct display_image *image, uint starty, uint endy)
{
    DEBUG_ASSERT(image);

    // Convert the framebuffer into something that the e-ink display will
    // understand.
    // TODO(gkalsi): Note that we're ignoring starty and endy right now. Just
    //               dump the whole display for now and we can worry about
    //               partial updates later.

    // memset(framebuffer, 0, FBSIZE);
    uint8_t *fb = (uint8_t *)framebuffer;
    uint8_t *px = (uint8_t *)image->pixels;
    for (unsigned int col = 0; col < 60; col++) {
        for (unsigned int row = 0; row < 240; row++) {
            fb[col + row * 60]  = clamp_px((px[col * 2 + row * 120] & 0xC0) >> 6) << 4;
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120] & 0x0C) >> 2) << 6;
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120 + 1] & 0xC0) >> 6);
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120 + 1] & 0x0C) >> 2) << 2;
        }
    }

    return eink_dumpfb(0,framebuffer, FBSIZE);
}

static void eink_flush(uint starty, uint endy)
{
    display_present(&eink_framebuffer.image, starty, endy);
}

status_t display_get_framebuffer(struct display_framebuffer *fb)
{
    DEBUG_ASSERT(fb);
    // Build the framebuffer.

    eink_framebuffer.image.format = IMAGE_FORMAT_MONO_8;
    eink_framebuffer.image.stride = HRES;
    eink_framebuffer.image.rowbytes = HRES;

    eink_framebuffer.image.pixels = malloc(HRES * VRES);
    eink_framebuffer.image.width = HRES;
    eink_framebuffer.image.height = HRES;
    eink_framebuffer.flush = eink_flush;
    eink_framebuffer.format = DISPLAY_FORMAT_RGB_111;   // TODO(gkalsi): This is not RGB, we're lying
    *fb = eink_framebuffer;
    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    DEBUG_ASSERT(info);

    info->format = IMAGE_FORMAT_MONO_8;
    info->width = HRES;
    info->height = HRES;

    return NO_ERROR;
}


