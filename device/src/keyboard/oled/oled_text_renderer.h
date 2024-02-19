#ifndef __OLED_TEXT_RENDERER_H__
#define __OLED_TEXT_RENDERER_H__

// Includes:

#include <inttypes.h>
#include "lvgl/lvgl.h"
#include "framebuffer.h"

// Macros:

// Variables:

// Functions:

void Framebuffer_DrawText(framebuffer_t* buffer, uint16_t x, uint16_t y, const lv_font_t* font, const char* text);

#endif
