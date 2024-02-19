#include "oled_buffer.h"
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "fonts/fonts.h"
#include "framebuffer.h"
#include "oled_text_renderer.h"

static struct {
    uint16_t width;
    uint16_t height;
    bool dirty;
    uint8_t buffer[DISPLAY_USABLE_WIDTH*DISPLAY_USABLE_HEIGHT];
} oledBuffer = { .width = DISPLAY_USABLE_WIDTH, .height = DISPLAY_USABLE_HEIGHT, .dirty = true };

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
    testingPattern();
    Framebuffer_DrawText(OledBuffer, 16, 16, &JetBrainsMono32, "Hello world!");
}

void Oled_LogConstant(const char* text)
{
    const lv_font_t* logFont = &CustomMono8;
    uint8_t line_height = logFont->line_height;
    Framebuffer_Shift(OledBuffer, line_height);
    Framebuffer_DrawText(OledBuffer, 0, OledBuffer->height-line_height, logFont, text);
}

void Oled_Log(const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);
    Oled_LogConstant(buffer);
}


