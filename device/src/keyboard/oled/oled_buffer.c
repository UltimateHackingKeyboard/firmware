#include "oled_buffer.h"
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "fonts/fonts.h"
#include "framebuffer.h"
#include "oled_text_renderer.h"
#include "oled.h"

framebuffer_t oledBuffer;

framebuffer_t* OledBuffer = (framebuffer_t*)&oledBuffer;

void OledBuffer_Init()
{
    Framebuffer_DrawText(NULL, OledBuffer, AlignmentType_Center, AlignmentType_Center, &JetBrainsMono32, "Hello world!", NULL);
}

