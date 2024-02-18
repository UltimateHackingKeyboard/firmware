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
void OledBuffer_Shift(uint8_t shiftBy);

#endif
