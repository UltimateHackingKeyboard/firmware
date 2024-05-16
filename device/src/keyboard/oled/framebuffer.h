#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

// Includes:

    #include "oled_display.h"
    #include <inttypes.h>
    #include <stdbool.h>
    #include <zephyr/sys/util.h>

// Macros:

// Typedefs:

    typedef enum {
        AnchorType_Begin,
        AnchorType_Center,
        AnchorType_End,
    } anchor_type_t;

    typedef struct {
        uint8_t value;
        uint8_t oldValue;
    } pixel_t;

    typedef struct {
        uint8_t min;
        uint8_t max;
    } range_t;

    typedef struct {
        pixel_t buffer[DISPLAY_WIDTH*DISPLAY_HEIGHT/2];
        range_t dirtyRanges[DISPLAY_HEIGHT];
    } framebuffer_t;

    typedef struct widget_t widget_t;

// Variables:

// Functions:

    void Framebuffer_Clear(widget_t* canvas, framebuffer_t* buffer);
    void Framebuffer_DrawHLine(widget_t* canvas, framebuffer_t* buffer, uint8_t x1, uint8_t x2, uint8_t y);
    void Framebuffer_DrawVLine(widget_t* canvas, framebuffer_t* buffer, uint8_t x, uint8_t y1, uint8_t y2);

    // todo: currently we use 8 bit colors; refactor it to 4 bits.
    static inline void Framebuffer_SetPixel(framebuffer_t* buffer, uint16_t x, uint16_t y, uint8_t value)
    {
        uint16_t index = (y*DISPLAY_WIDTH+x)/2;
        if (x%2 == 1) {
            buffer->buffer[index].value = (buffer->buffer[index].value & 0x0f) | (value & 0xf0);
        } else {
            buffer->buffer[index].value = (buffer->buffer[index].value & 0xf0) | (value >> 4);
        }
        buffer->dirtyRanges[y].min = MIN(buffer->dirtyRanges[y].min, x/2);
        buffer->dirtyRanges[y].max = MAX(buffer->dirtyRanges[y].max, x/2);
    };

    static inline pixel_t* Framebuffer_GetPixel(framebuffer_t* buffer, uint16_t x, uint16_t y)
    {
        uint16_t index = (y*DISPLAY_WIDTH+x)/2;
        return &buffer->buffer[index];
    };

#endif
