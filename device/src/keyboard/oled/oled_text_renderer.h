#ifndef __OLED_TEXT_RENDERER_H__
#define __OLED_TEXT_RENDERER_H__

// Includes:

    #include <inttypes.h>
    #include "lvgl/lvgl.h"
    #include "framebuffer.h"

// Macros:

// Variables:

// Typedefs:

    typedef struct widget_t widget_t;

// Functions:

    void Framebuffer_DrawTextAnchored(widget_t* canvas, framebuffer_t* buffer, anchor_type_t horizontalAnchor, anchor_type_t verticalAnchor, const lv_font_t* font, const char* text);
    void Framebuffer_DrawText(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, const char* text);

#endif
