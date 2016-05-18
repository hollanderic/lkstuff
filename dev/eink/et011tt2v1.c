#include <assert.h>
#include <err.h>
#include <debug.h>
#include <rand.h>
#include <stdlib.h>
#include <trace.h>
#include <platform/timer.h>
#include <platform/gpio.h>
#include <target/gpioconfig.h>
#include <target/tqlogo.h>
#include <dev/et011tt2v1.h>
#include <target/spi.h>
#include <string.h>
// TODO The eink driver should not include stm headers. We likely need INIT to store
// a spihandle and then spi functions use it some other way
#include <stm32f7xx.h>
#include <platform.h>

/* The following tables are copied verbatim with comments from the verily driver */
// TODO(nicholasewalt): Update LUTs once they are provided by Eink.
// TODO(nicholasewalt): Investigate truncating and compressing LUTs. Current
// tables are only 62 frames long and are highly compressible, however new
// tables will likely have less potential gains.
// FTLUT
// White REAGL LUT for K/W/Gray mode.
//
// F/T waveform for up to 128 frames, 1 byte per frame.
//
// For one frame:
//
// |    D7  D6    |    D5  D4    |    D3  D2    |    D1  D0    |
// |--------------+--------------+--------------+--------------|
// |  F1_KF[1:0]  |  F1_KT[1:0]  |  F1_WF[1:0]  |  F1_WT[1:0]  |
//
// NOTE: These bit definitions are not explained in more detail in the
// datasheet.
//
static const uint8_t lut_ft[128] = {
    0x55, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
    0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x5A, 0x5A,
    0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A, 0x5A,
    0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// KWG_VCOM
// VCOM LUT for K/W/Gray mode.
//
// VCOM LUT for up to 128 frames, 2 bits per frame.
//
// For one frame:
//
// |    D7  D6    |    D5  D4    |    D3  D2    |    D1  D0    |
// |--------------+--------------+--------------+--------------|
// |  VCOM1[1:0]  |  VCOM2[1:0]  |  VCOM3[1:0]  |  VCOM4[1:0]  |
//
// VCOM(1~128)[1:0]: VCOM voltage level of Frame 1~128, respectively.
//  00b: VCOM output VCOMDC
//  01b: VCOM output VSH+VCOM_DC (VCOMH)
//  10b: VCOM output VSL+VCOM_DC (VCOML)
//  11b: VCOM output floating.
//
static const uint8_t lut_kwg_vcom[32] = {
    0x55, 0x6A, 0xA5, 0x55, 0x55, 0x55, 0x55, 0x55, 0x56, 0xAA, 0xAA,
    0xAA, 0xAA, 0xAA, 0xAA, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
// KWG_LUT
// Source LUT for K/W/Gray mode.
// NOTE: This LUT was ripped from the Hulk Jig firmware, however it did not
// include waveforms for transitioning into or out of gray*. nicholasewalt@
// added preliminary white to gray* transitions, though they do not have good
// mixing and other gray* transitions are still unsupported.
//
// Pixel waveforms for up to 128 frames, 4 bytes per frame.
// If waveforms of one frame are all 11b, the update precedue will stop.
//
// For one frame:
//
// |    D7  D6    |    D5  D4    |    D3  D2    |    D1  D0    |
// |--------------+--------------+--------------+--------------|
// | F1_P0C0[1:0] | F1_P0C1[1:0] | F1_P0C2[1:0] | F1_P0C3[1:0] |
// | F1_P1C0[1:0] | F1_P1C1[1:0] | F1_P1C2[1:0] | F1_P1C3[1:0] |
// | F1_P2C0[1:0] | F1_P2C1[1:0] | F1_P2C2[1:0] | F1_P2C3[1:0] |
// | F1_P3C0[1:0] | F1_P3C1[1:0] | F1_P3C2[1:0] | F1_P3C3[1:0] |
//
// P0C0/P0C1/P0C2/P0C3: black to black/gray1/gray2/white respectively
// P1C0/P1C1/P1C2/P1C3: gray1 to black/gray1/gray2/white respectively
// P2C0/P2C1/P2C2/P2C3: gray2 to black/gray1/gray2/white respectively
// P3C0/P3C1/P3C2/P3C3: white to black/gray1/gray2/white respectively
//
// P0~3C0~3[1:0]:
//  00b: GND
//  01b: VSH
//  10b: VSL
//  11b: VSHR
//
static const uint8_t lut_kwg[512] = {
    0x41, 0x81, 0x81, 0x81, 0x41, 0x81, 0x81, 0x81, 0x41, 0x81, 0x81, 0x81,
    0x41, 0x81, 0x81, 0x81, 0x41, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0x82,
    0x81, 0x81, 0x81, 0x82, 0x81, 0x81, 0x81, 0x82, 0x81, 0x81, 0x81, 0x82,
    0x81, 0x81, 0x81, 0x82, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41,
    0x42, 0x42, 0x42, 0x41, 0x42, 0x42, 0x42, 0x41, 0x82, 0x42, 0x42, 0x42,
    0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42,
    0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42,
    0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42, 0x82, 0x42, 0x42, 0x42,
    0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a,
    0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a,
    0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a, 0x82, 0x42, 0x42, 0x4a,
    0x82, 0x42, 0x42, 0x6a, 0x82, 0x42, 0x42, 0x6a, 0x82, 0x42, 0x42, 0x6a,
    0x82, 0x42, 0x42, 0x6a, 0x82, 0x42, 0x42, 0x6a, 0x82, 0x42, 0x42, 0x6a,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

/* cja: end of imported LUTs */
#define LOCAL_TRACE 0




spin_lock_t lock;
spin_lock_saved_state_t state;



static bool _poll_gpio(uint32_t gpio, bool desired, uint32_t timeout)

{
    lk_time_t now = current_time();
    uint32_t current;

    while ((current = gpio_get(gpio)) != desired) {
        if (current_time() - now > timeout) {
            break;
        }
    }

    return (current == desired);
}

/* The display pulls the BUSY line low while BUSY and releases it when done */
static bool _wait_not_busy(eink_t * disp_p) {
    if (disp_p->with_busy) {
        return _poll_gpio(disp_p->busy_gpio,1,EINK_BUSY_TIMEOUT);
    } else {
        return true;
    }
}

static inline void _assert_reset(eink_t * disp_p) {
    gpio_set(disp_p->rst_gpio, 0);
}

static inline void _release_reset(eink_t * disp_p) {
    gpio_set(disp_p->rst_gpio, 1);
}


void _write_cmd(eink_t * disp_p, uint8_t cmd) {

    uint16_t cmd16;
    cmd16 = (uint16_t) cmd;
    spi_write(disp_p->spi_handle,(uint8_t *)&cmd16, 1 , disp_p->cs_gpio); //Since we are 9-bin transfers, need to send as 16bit val
}


/* _write_byte_data assumes the image buffer contains 8 bit per pixel information, must be repacked as 2bit (4 pixel per byte) */

void _write_byte_data(eink_t * disp_p, const uint8_t * buf, size_t len) {

    uint16_t txbyte;


    for (size_t i = 0; i < (len>>2); i++) {
        txbyte = ((uint16_t) (buf[4*i     ] & 0x03) << 6) | 
                 ((uint16_t) (buf[4*i + 1 ] & 0x03) << 4) | 
                 ((uint16_t) (buf[4*i + 2 ] & 0x03) << 2) | 
                 ((uint16_t) (buf[4*i + 3 ] & 0x03) << 0) | 0x0100;
        spi_write(disp_p->spi_handle, (uint8_t *)&txbyte,1, disp_p->cs_gpio);
    }
}


void _write_data(eink_t * disp_p, const uint8_t * buf, size_t len) {

    uint16_t txbyte;

    for (size_t i = 0; i < len; i++) {
        txbyte = ((uint16_t) buf[i]) | 0x0100;
        spi_write((void *)disp_p->spi_handle, (uint8_t *)&txbyte,1, disp_p->cs_gpio);
    }
}

void _write_param(eink_t * disp_p, uint8_t param) {

    uint16_t data16;
    data16 = (uint16_t) param | 0x0100;
    spi_write((void *)disp_p->spi_handle, (uint8_t *)&data16,1, disp_p->cs_gpio);
}

/*status_t read_data(uint8_t *buf, size_t len) {
    return spi_read(&SpiHandle, buf, len, GPIO_DISP0_CS);
}

status_t get_status(et011tt2_status_t *status) {
    status_t err;

    if (status == NULL) {
        return ERR_INVALID_ARGS;
    }

    _write_cmd(0,GetStatus);
    err = read_data((uint8_t *) status, sizeof(et011tt2_status_t));

    return err;
}
*/



status_t eink_disp_init(eink_t * disp_p){

    TRACE_ENTRY;
    status_t err = NO_ERROR;

    if (!_wait_not_busy(disp_p)) {
        printf("Device is still busy after initial reset!\n");
        return ERR_GENERIC;
    }


    // Configure power settings
    _write_cmd(disp_p, PowerSetting);
    _write_param(disp_p, 0x03);
    _write_param(disp_p, 0x00);
    _write_param(disp_p, 0x1c);
    _write_param(disp_p, 0x1c);
    _write_param(disp_p, 0x00);

    // Power on display
    _write_cmd(disp_p, PowerOn);

    // Configure panel settings
    _write_cmd(disp_p, PanelSetting);
    _write_param(disp_p, 0x0f);
    _write_param(disp_p, 0x86);

    // Configure Boost
    _write_cmd(disp_p, BoosterSoftStart);
    _write_param(disp_p, 0x3e);
    _write_param(disp_p, 0x3e);
    _write_param(disp_p, 0x3e);

    // Initialize -> Check_Busy
    _write_cmd(disp_p, VcomAndDataIntervalSetting);
    _write_param(disp_p, 0x01);
    _write_param(disp_p, 0x20);
    _write_param(disp_p, 0x10);

    _write_cmd(disp_p, ResolutionSetting);
    _write_param(disp_p,  (EINK_HRES-1) | 0x03);
    _write_param(disp_p,  (EINK_VRES-1) >> 8 );
    _write_param(disp_p,  (EINK_VRES-1) & 0xFF);


    _write_cmd(disp_p, GateGroupSetting);
    _write_param(disp_p, 0x89);
    _write_param(disp_p, 0x89);
    _write_param(disp_p, 0xcb);
    _write_param(disp_p, 0xcb);
    _write_param(disp_p, 0x03);

    _write_cmd(disp_p, BorderDcVoltageSetting);
    _write_param(disp_p, 0x4e);

    _write_cmd(disp_p, LpdSelect);
    _write_param(disp_p, 0x02);

    _write_cmd(disp_p, FtLutRegister);
    _write_data(disp_p, lut_ft, sizeof(lut_ft));

    _write_cmd(disp_p, KwgVcomLutRegister);
    _write_data(disp_p, lut_kwg_vcom, sizeof(lut_kwg_vcom));

    _write_cmd(disp_p, KwgLutRegister);
    _write_data(disp_p, lut_kwg, sizeof(lut_kwg));

    _write_cmd(disp_p, VcomDcSetting);
    _write_param(disp_p, 0x22);


    //if (!check_busy(disp_p)) {
    //    printf("Device is still busy after Power On and configuration\n");
    //    return ERR_GENERIC;
    //}

    memset(disp_p->framebuff, 0xff, EINK_FBSIZE);
    return eink_refresh(disp_p);

err:
    TRACE_EXIT;
    return err;
}

status_t eink_init(eink_t * disp_p) {
    TRACE_ENTRY;
    status_t err = NO_ERROR;
/*
    if (!disp_p->spi_inited) {
        SpiHandle.Instance               = disp_p->spi;
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
        disp_p->spi_inited = true;
    }
*/
    // VDD (wired straight to 3v3)
    spin(2000);         // Delay 2 ms
    _assert_reset(disp_p);     // RST_LOW
    spin(30);           // Delay 30 us
    _release_reset(disp_p);    // RST_HIGH
    eink_disp_init(disp_p);

err:
    TRACE_EXIT;
    return err;
}

static int _eink_startdatawindow(eink_t * disp_p) {
    _write_cmd(disp_p, DataStartTransmissionWindow);
    _write_param(disp_p, 0x00);
    _write_param(disp_p, 0x00);
    _write_param(disp_p, 0x00);
    _write_param(disp_p,  (EINK_HRES -1) | 0x03 );
    _write_param(disp_p,  (EINK_VRES -1) >> 8 );
    _write_param(disp_p,  (EINK_VRES -1) & 0xff);
    return 0; 
}
static int _eink_displayrefresh(eink_t * disp_p) {
    _write_cmd(disp_p, DisplayRefresh);
    _write_param(disp_p, 0x00);
    _write_param(disp_p, 0x00);
    _write_param(disp_p, 0x00);
    _write_param(disp_p, 0x00);
    _write_param(disp_p,  (EINK_HRES -1) | 0x03 );
    _write_param(disp_p,  (EINK_VRES -1) >> 8 );
    _write_param(disp_p,  (EINK_VRES -1) & 0xff);
    return 0;
}

int eink_dumpfb(eink_t * disp_p)
{
    if (disp_p->framebuff) {

        _eink_startdatawindow(disp_p);

        _write_cmd(disp_p, DataStartTransmission2);
        _write_byte_data(disp_p, disp_p->framebuff, EINK_FBSIZE);

        _eink_displayrefresh(disp_p);

        return 0;
   }
    return -1;
}
/* eink_drawimagebuff assumes the buffer contains 2 bit per pixel data */
int eink_drawimagebuff(eink_t * disp_p, uint8_t * buff){
    if (buff) {

        _eink_startdatawindow(disp_p);

        _write_cmd(disp_p, DataStartTransmission2);
        _write_data(disp_p, buff, EINK_FBSIZE);

        _eink_displayrefresh(disp_p);

        return 0;
   }
    return -1;
}

int eink_refresh(eink_t * disp_p) {
    return eink_dumpfb(disp_p);
}


uint8_t * get_eink_framebuffer(eink_t * disp_p) {
    return disp_p->framebuff;
}



