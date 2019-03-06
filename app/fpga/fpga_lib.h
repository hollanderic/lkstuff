
/*
    the display memory in the display module is 4 bits per pixel, so each byte contains
    data for two pixels
*/

#pragma once

void fpga_init(void);
//void ws_eink_display(const uint8_t *blackimage, const uint8_t *redimage);
void fpga_test(void);
void fpga_fbuf_refresh(void);


