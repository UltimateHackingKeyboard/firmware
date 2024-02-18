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
            OledBuffer[i][j] = 0x70+j/2;
        }
    }
}

void OledBuffer_Init()
{
    testingPattern();
    Oled_DrawText(16, 16, &JetBrainsMono32, "Hello world!");
}

void OledBuffer_Shift(uint8_t shiftBy)
{
    if (shiftBy > DISPLAY_HEIGHT) {
        shiftBy = DISPLAY_HEIGHT;
    }

    memcpy(&OledBuffer[0][0], &OledBuffer[shiftBy][0], (DISPLAY_HEIGHT-(uint16_t)shiftBy)*DISPLAY_WIDTH);
    memset(&OledBuffer[DISPLAY_HEIGHT-shiftBy][0], 0, shiftBy*DISPLAY_WIDTH);
}
