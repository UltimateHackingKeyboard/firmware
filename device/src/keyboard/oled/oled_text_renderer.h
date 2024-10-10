#ifndef __OLED_TEXT_RENDERER_H__
#define __OLED_TEXT_RENDERER_H__

// Includes:

    #include <inttypes.h>
    #include "lvgl/lvgl.h"
    #include "framebuffer.h"
    #include "keyboard/oled/fonts/font_awesome_12.h"

// Macros:

// Variables:

// Typedefs:

    typedef enum {
        FontControl_NextCharGray = 1,
        FontControl_NextCharWhite,
        FontControl_NextCharBlack,
        FontControl_NextCharIcon12,
    } font_control_t;

    typedef struct widget_t widget_t;

// Functions:

    void Framebuffer_DrawText(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, const char* text, const char* textEnd);
    uint16_t Framebuffer_GetGlyphWidth(const lv_font_t* font, uint8_t glyphIdx);
    uint16_t Framebuffer_TextWidth(const lv_font_t* font, const char* text, const char* textEnd, uint16_t maxWidth, bool* truncated, const char** truncatedText);

#endif
