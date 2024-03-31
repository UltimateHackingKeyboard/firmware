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
    uint8_t buffer[DISPLAY_WIDTH*DISPLAY_HEIGHT];
} oledBuffer = { .width = DISPLAY_WIDTH, .height = DISPLAY_HEIGHT };

framebuffer_t* OledBuffer = (framebuffer_t*)&oledBuffer;

void OledBuffer_Init()
{
    Framebuffer_DrawTextAnchored(NULL, OledBuffer, AnchorType_Center, AnchorType_Center, &JetBrainsMono32, "Hello world!");
}

