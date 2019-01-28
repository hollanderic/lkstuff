/******************************************************************************
**************************Hardware interface layer*****************************
* | file      	:	DEV_Config.h
* |	version		:	V1.0
* | date		:	2017-08-14
* | function	:	
	Provide the hardware underlying interface	
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_
#include <platform.h>
#include <dev/gpio.h>
#include "pin_config.h"

#define USE_SPI_4W 1
#define USE_IIC 0

#define IIC_CMD        0X00
#define IIC_RAM        0X40

//OLED GPIO
#define OLED_CS_0		gpio_set(CS_PIN, 0)
#define OLED_CS_1		gpio_set(CS_PIN, 1)

#define OLED_DC_0		gpio_set(DC_PIN, 0)
#define OLED_DC_1		gpio_set(DC_PIN, 1)

#define OLED_RST_0		gpio_set(RST_PIN, 0)
#define OLED_RST_1		gpio_set(RST_PIN, 1)

//SPI GPIO
#define SPI1_SCK_0		gpio_set(SCK_PIN, 0)
#define SPI1_SCK_1		gpio_set(SCK_PIN, 1)

#define SPI1_MOSI_0		gpio_set(MOSI_PIN, 0)
#define SPI1_MOSI_1		gpio_set(MOSI_PIN, 1)
/*------------------------------------------------------------------------------------------------------*/

uint8_t System_Init(void);
void    System_Exit(void);

uint8_t SPI4W_Write_Byte(uint8_t value);

void Driver_Delay_ms(uint32_t xms);
void Driver_Delay_us(uint32_t xus);

#endif
