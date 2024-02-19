#include "oled_buffer.h"
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "fonts/fonts.h"
#include "framebuffer.h"
#include "oled_text_renderer.h"
#include "oled.h"

static struct {
    uint16_t width;
    uint16_t height;
    uint8_t buffer[DISPLAY_USABLE_WIDTH*DISPLAY_USABLE_HEIGHT];
} oledBuffer = { .width = DISPLAY_USABLE_WIDTH, .height = DISPLAY_USABLE_HEIGHT };

framebuffer_t* OledBuffer = (framebuffer_t*)&oledBuffer;

static void testingPattern()
{
    for (uint16_t x = 0; x < OledBuffer->width; x++) {
        for (uint16_t y = 0; y < OledBuffer->height; y++) {
            //Framebuffer_SetPixel(OledBuffer, x, y, 0x70+x/2);
            Framebuffer_SetPixel(OledBuffer, x, y, x == y ? 0xff : 0x00);
        }
    }
}

void OledBuffer_Init()
{
    // testingPattern();
    //Framebuffer_DrawText(NULL, OledBuffer, 16, 16, &JetBrainsMono32, "Hello world!");
}

