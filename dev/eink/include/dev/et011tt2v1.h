
#include <kernel/spinlock.h>


#define EINK_HRES	240
#define	EINK_VRES	240
#define EINK_FBSIZE  EINK_HRES*EINK_VRES>>2

#define EINK_WHITE      0xFF
#define EINK_BLACK      0x00


typedef struct {
    uint32_t    spi;
    uint32_t    cs_gpio;
    uint32_t    busy_gpio;
    uint32_t    rst_gpio
    bool        with_busy;
    bool        spi_inited;
    spin_lock_t spi_lock;
    uint8_t *   framebuff_p;
} eink_t;


int eink_refresh(uint8_t disp_num);

status_t eink_init(void);

uint8_t * get_eink_framebuffer(void);        // returns pointer to eink framebuffer


enum {
    PanelSetting                = 0x00,
    PowerSetting                = 0x01,
    PowerOff                    = 0x02,
    PowerOffSequenceSetting     = 0x03,
    PowerOn                     = 0x04,
    BoosterSoftStart            = 0x06,
    DeepSleep                   = 0x07,
    DataStartTranmission1       = 0x10,
    DisplayRefresh              = 0x12,
    DataStartTransmission2      = 0x13,
    DataStartTransmissionWindow = 0x14,
    KwgVcomLutRegister          = 0x20,
    KwgLutRegister              = 0x22,
    FtLutRegister               = 0x26,
    PllControl                  = 0x30,
    TemperatureSensor           = 0x40,
    TemperatureSensorEnable     = 0x41,
    TemperatureSensorWrite      = 0x42,
    TemperatureSensorRead       = 0x43,
    VcomAndDataIntervalSetting  = 0x50,
    LowPowerDetection           = 0x51,
    ResolutionSetting           = 0x61,
    GateGroupSetting            = 0x62,
    GateBlockSetting            = 0x63,
    GateSelectSetting           = 0x64,
    Revision                    = 0x70,
    GetStatus                   = 0x71,
    AutoMeasureVcom             = 0x80,
    VcomValue                   = 0x81,
    VcomDcSetting               = 0x82,
    BorderDcVoltageSetting      = 0x84,
    LpdSelect                   = 0xE4,
  };


enum booster_soft_start_min_off {
    soft_start_min_off_0p27us = 0b000,
    soft_start_min_off_0p34us = 0b001,
    soft_start_min_off_0p40us = 0b010,
    soft_start_min_off_0p50us = 0b011,
    soft_start_min_off_0p80us = 0b100,
    soft_start_min_off_1p54us = 0b101,
    soft_start_min_off_3p34us = 0b110,
    soft_start_min_off_6p58us = 0b111,
};

enum drive_strength {
    drive_strength_1 = 0b000,
    drive_strength_2 = 0b001,
    drive_strength_3 = 0b010,
    drive_strength_4 = 0b011,
    drive_strength_5 = 0b100,
    drive_strength_6 = 0b101,
    drive_strength_7 = 0b110,
    drive_strength_8 = 0b111,  // (strongest)
};

enum soft_start_period {
    soft_start_period_10ms = 0b00,
    soft_start_period_20ms = 0b01,
    soft_start_period_30ms = 0b10,
    soft_start_period_40ms = 0b11,
};

enum lpd_select_lpdsel {
    LPDSEL_2p2v = 0b00,
    LPDSEL_2p3v = 0b01,
    LPDSEL_2p4v = 0b10,
    LPDSEL_2p5v = 0b11, // (default)
};

