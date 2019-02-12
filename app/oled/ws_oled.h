
/*
    the display memory in the display module is 4 bits per pixel, so each byte contains
    data for two pixels
*/

#pragma once

#define OLED_WIDTH 128
#define OLED_HEIGHT 128

void ws_oled_init(void);
void ws_oled_display(void);

