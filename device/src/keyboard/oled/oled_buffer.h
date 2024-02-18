#ifndef __OLED_BUFFER_H__
#define __OLED_BUFFER_H__

// Includes:

#include <inttypes.h>
#include <stdbool.h>

// Macros:

#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 64

// Variables:

extern volatile bool OledBuffer_NeedsRedraw;
extern uint8_t OledBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

// Functions:

void OledBuffer_Init();
void OledBuffer_Shift(uint16_t shiftBy);

static inline void OledBuffer_SetPixel(uint16_t x, uint16_t y, uint8_t value)
{
    if ((OledBuffer[y][x] & 0xf0) != (value & 0xf0) && x < DISPLAY_WIDTH && y < DISPLAY_HEIGHT) {
        OledBuffer[y][x] = (value & 0xf0) | 0x01;
    }
};

#endif
