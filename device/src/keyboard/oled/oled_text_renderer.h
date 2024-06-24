#ifndef __OLED_TEXT_RENDERER_H__
#define __OLED_TEXT_RENDERER_H__

// Includes:

    #include <inttypes.h>
    #include "lvgl/lvgl.h"
    #include "framebuffer.h"

// Macros:

// Variables:

// Typedefs:

    typedef enum {
        FontControl_NextCharGray = 1,
        FontControl_NextCharWhite,
        FontControl_NextCharIcon12,
    } font_control_t;

    typedef enum {
        FontIcon_CircleXmarkLarge = 32,
        FontIcon_CircleXmark,
        FontIcon_Plug,
        FontIcon_SignalStream,
    } font_icons_t;

    typedef struct widget_t widget_t;

// Functions:

    void Framebuffer_DrawText(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, const char* text, const char* textEnd);
    uint16_t Framebuffer_TextWidth(const lv_font_t* font, const char* text, const char* textEnd);

#endif
