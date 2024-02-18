#ifndef __OLED_BUFFER_H__
#define __OLED_BUFFER_H__

// Includes:

#include <inttypes.h>

// Macros:

#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 64

// Variables:

extern uint8_t OledBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

// Functions:

void OledBuffer_Init();

#endif
