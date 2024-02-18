#include "oled_buffer.h"
#include <inttypes.h>
#include <string.h>
#include "fonts/fonts.h"
#include "oled_text_renderer.h"

uint8_t OledBuffer[DISPLAY_HEIGHT][DISPLAY_WIDTH];
volatile bool OledBuffer_NeedsRedraw = true;

static void testingPattern()
{
    for (uint16_t i = 0; i < DISPLAY_HEIGHT; i++) {
        for (uint16_t j = 0; j < DISPLAY_WIDTH; j++) {
            OledBuffer_SetPixel(j, i, 0x70+j/2);
        }
    }
}

void OledBuffer_Init()
{
    testingPattern();
    Oled_DrawText(16, 16, &JetBrainsMono32, "Hello world!");

}

void OledBuffer_Shift(uint16_t shiftBy)
{
    if (shiftBy > DISPLAY_HEIGHT) {
        shiftBy = DISPLAY_HEIGHT;
    }

    for (uint16_t y = 0; y < DISPLAY_HEIGHT-shiftBy; y++) {
        for (uint16_t x = 0; x < DISPLAY_WIDTH; x++) {
            OledBuffer_SetPixel(x, y, OledBuffer[y+shiftBy][x]);
        }
    }
    for (uint16_t y = DISPLAY_HEIGHT - shiftBy; y < DISPLAY_HEIGHT; y++) {
        for (uint16_t x = 0; x < DISPLAY_WIDTH; x++) {
            OledBuffer_SetPixel(x, y, 0);
        }
    }
}

