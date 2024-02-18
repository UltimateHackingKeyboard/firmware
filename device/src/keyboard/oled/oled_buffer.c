#include "oled_buffer.h"
#include <inttypes.h>

uint8_t OledBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];

static void testingPattern() {
    for (uint16_t i = 0; i < DISPLAY_HEIGHT; i++) {
        if (i < 32) {
            for (uint16_t j = 0; j < DISPLAY_WIDTH; j++) {
                OledBuffer[i][j] = j;
            }
        } else {
            for (uint16_t j = 0; j < DISPLAY_WIDTH; j++) {
                OledBuffer[i][j] = 0;
            }
        }
    }
}

void OledBuffer_Init() {
    testingPattern();
}
