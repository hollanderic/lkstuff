/******************************************************************************
**************************Hardware interface layer*****************************
* | file      	:	DEV_Config.c
* |	version		:	V1.0
* | date		:	2017-08-14
* | function	:	
	Provide the hardware underlying interface	
******************************************************************************/

#include <stdio.h>		//printf()
#include <platform/spim.h>
#include <platform.h>
#include "DEV_Config.h"

#include "pin_config.h"

int spi_callback(int input);
static nrf_spim_dev_t spim0 = {
    .instance = NRF_SPIM0,
    .sclk_pin = SCK_PIN,
    .mosi_pin = MOSI_PIN,
    .miso_pin = 0x80000000,
    .speed = SPIM_SPEED_8M,
    .cb = spi_callback };

int spi_callback(int input) {
#if 0
    gpio_set(LED_OE, 1);
    gpio_set(LED_STRB, 1);
    gpio_set(LED_A, (channel & 0x01) ? 1 : 0);
    gpio_set(LED_B, (channel & 0x02) ? 1 : 0);
    gpio_set(LED_C, (channel & 0x04) ? 1 : 0);
    gpio_set(LED_D, (channel & 0x08) ? 1 : 0);

    gpio_set(LED_STRB, 0);
    gpio_set(LED_OE, 0);
    nrf_spim_send(&spim0, fbuf, 8);
#endif
    return 0;
}

/********************************************************************************
function:	System Init
note:
	Initialize the communication method
********************************************************************************/
uint8_t System_Init(void)
{
    nrf_spim_init(&spim0);

    printf("USE 4wire spi\r\n");
    return 0;
}

void System_Exit(void)
{

}
/********************************************************************************
function:	Hardware interface
note:
	SPI4W_Write_Byte(value) : 
		HAL library hardware SPI
		Register hardware SPI
		Gpio analog SPI
	I2C_Write_Byte(value, cmd):
		HAL library hardware I2C
********************************************************************************/
uint8_t SPI4W_Write_Byte(uint8_t value)
{
    nrf_spim_send(&spim0, &value, 1);
    return 1;
}

/********************************************************************************
function:	Delay function
note:
	Driver_Delay_ms(xms) : Delay x ms
	Driver_Delay_us(xus) : Delay x us
********************************************************************************/
void Driver_Delay_ms(uint32_t xms)
{
    Driver_Delay_us(xms*1000);
}

void Driver_Delay_us(uint32_t xus)
{
    int j;
    for(j=xus; j > 0; j--);
}
