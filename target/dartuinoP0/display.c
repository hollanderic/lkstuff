#include <debug.h>
#include <trace.h>
#include <err.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>


#include <dev/display.h>
#include <dev/et011tt2v1.h>
#include <target/gpioconfig.h>
#include <platform/stm32.h>
#include <platform/gpio.h>
#include <target/spi.h>
#include <target/tqlogo.h>
#include <target/baby.h>



static struct display_framebuffer eink_framebuffer;

static eink_t eink_display;
static uint8_t eink_fb[EINK_FBSIZE];

static SPI_HandleTypeDef SpiHandle;


static uint8_t clamp_px(uint8_t px)
{
    if (px) return 0x3;
    return 0x0;
}


int display_init(void *fb){
 	status_t err = NO_ERROR;
	GPIO_InitTypeDef  GPIO_InitStruct;

    printf("Configuring SPI2.\n");
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOK_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();




    // GPIO_EINK_CS - B,12
    GPIO_InitStruct.Mode      = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Pin       = GPIO_PIN_12;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

    /* GPIO_EINK_DC - I,5 */
    GPIO_InitStruct.Pin       = GPIO_PIN_5;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_5, GPIO_PIN_RESET);

    /* GPIO_EINK_RST K,6 */
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    HAL_GPIO_Init(GPIOK, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOK, GPIO_PIN_6, GPIO_PIN_RESET);


    /* GPIO_EINK_BUSY I,4 */
    GPIO_InitStruct.Mode      = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Pin       = GPIO_PIN_4;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    /* Common SPI2 AF config */
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = 0x2; // GPIO_PULLDOWN
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    /* SPI2 MOSI GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_15;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    /* SPI2 SCK GPIO pin configuration */
    GPIO_InitStruct.Pin       = GPIO_PIN_3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* NVIC for SPI */
    HAL_NVIC_EnableIRQ(SPI2_IRQn);

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
    eink_display.framebuff = eink_fb;
    eink_display.cs_gpio   = GPIO_EINK_CS;
    eink_display.rst_gpio  = GPIO_EINK_RST;
    eink_display.busy_gpio = GPIO_EINK_BUSY;

	eink_init(&eink_display);
	printf("Display initialized! spi handle = 0x%08x\n");
	return 0;

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
    /*uint8_t *fb = (uint8_t *)framebuffer;
    uint8_t *px = (uint8_t *)image->pixels;
    for (unsigned int col = 0; col < 60; col++) {
        for (unsigned int row = 0; row < 240; row++) {
            fb[col + row * 60]  = clamp_px((px[col * 2 + row * 120] & 0xC0) >> 6) << 4;
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120] & 0x0C) >> 2) << 6;
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120 + 1] & 0xC0) >> 6);
            fb[col + row * 60] |= clamp_px((px[col * 2 + row * 120 + 1] & 0x0C) >> 2) << 2;
        }
    }
	*/
    return eink_drawimagebuff(&eink_display, (uint8_t *) image->pixels);
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
    eink_framebuffer.image.stride = EINK_HRES;
    eink_framebuffer.image.rowbytes = EINK_HRES;

    eink_framebuffer.image.pixels = malloc(EINK_HRES * EINK_VRES);
    eink_framebuffer.image.width = EINK_HRES;
    eink_framebuffer.image.height = EINK_VRES;
    eink_framebuffer.flush = eink_flush;
    eink_framebuffer.format = DISPLAY_FORMAT_RGB_111;   // TODO(gkalsi): This is not RGB, we're lying
    *fb = eink_framebuffer;
    return NO_ERROR;
}

status_t display_get_info(struct display_info *info)
{
    DEBUG_ASSERT(info);

    info->format = IMAGE_FORMAT_MONO_8;
    info->width = EINK_HRES;
    info->height = EINK_HRES;

    return NO_ERROR;
}



#if WITH_LIB_CONSOLE
#include <lib/console.h>


static int cmd_eink_fill(int argc, const cmd_args *argv)
{

    if (!(eink_display.framebuff==(uint8_t *)NULL) ){
        uint16_t x,y,count,val;
        x = argv[1].i;
        y = argv[2].i;
        val = argv[3].i;
        count = argv[4].i;
        memset(eink_display.framebuff + x + y*(240>>2), val,count);
        return eink_dumpfb(&eink_display);

    } else {
        return -1;
    }
}

static int cmd_eink1(int argc, const cmd_args *argv)
{
    memset(eink_display.framebuff, 0xff, EINK_FBSIZE);
    return eink_dumpfb(&eink_display);

}

static int cmd_eink0(int argc, const cmd_args *argv)
{
    memset(eink_display.framebuff, 0x00, EINK_FBSIZE);
    return eink_dumpfb(&eink_display);
}


static int cmd_eink_logo(int argc, const cmd_args *argv)
{

    return eink_drawimagebuff(&eink_display,logo);
}
static int cmd_eink_baby(int argc, const cmd_args *argv)
{

    return eink_drawimagebuff(&eink_display,baby);
}
static int cmd_eink_pattern(int argc, const cmd_args *argv)
{
	memset(eink_display.framebuff, 0x00, EINK_FBSIZE >> 2);
	memset(eink_display.framebuff + (EINK_FBSIZE >> 2), 0x01, EINK_FBSIZE >> 2);
	memset(eink_display.framebuff + (EINK_FBSIZE >> 1), 0x02, EINK_FBSIZE >> 2);
	memset(eink_display.framebuff + (3*(EINK_FBSIZE >> 2)),0x03, EINK_FBSIZE >> 2);

    return eink_dumpfb(&eink_display);
}

static int cmd_eink(int argc, const cmd_args *argv)
{
    return eink_init(&eink_display);
}


STATIC_COMMAND_START
STATIC_COMMAND("eink", "eink init", &cmd_eink)
STATIC_COMMAND("eink1", "eink fill white", &cmd_eink1)
STATIC_COMMAND("eink0", "eink fill black", &cmd_eink0)
STATIC_COMMAND("einkfill", "eink fill x y val count", &cmd_eink_fill)
STATIC_COMMAND("einklogo", "tqlogo", &cmd_eink_logo)
STATIC_COMMAND("baby", "baby", &cmd_eink_baby)

STATIC_COMMAND("einkpattern", "tqlogo", &cmd_eink_pattern)
STATIC_COMMAND_END(eink);

#endif




