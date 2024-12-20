#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

// Includes:

    #include "oled_display.h"
    #include <inttypes.h>
    #include <stdbool.h>
    #include "led_display.h"
    #include <zephyr/sys/util.h>

// Macros:

// Typedefs:

    typedef enum {
        AlignmentType_Begin_ = 1,
        AlignmentType_Center_ = 2,
        AlignmentType_End_ = 3,

        AlignmentType_Begin = -((AlignmentType_Begin_ << 9) + 256),
        AlignmentType_Center = -((AlignmentType_Center_ << 9) + 256),
        AlignmentType_End = -((AlignmentType_End_ << 9) + 256),
    } alignment_type_t;

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
        uint8_t shadedValue = value >> 4;
        if (x%2 == 1) {
            buffer->buffer[index].value = (buffer->buffer[index].value & 0x0f) | (shadedValue << 4);
        } else {
            buffer->buffer[index].value = (buffer->buffer[index].value & 0xf0) | (shadedValue & 0x0f);
        }
    };

    static inline pixel_t* Framebuffer_GetPixel(framebuffer_t* buffer, uint16_t x, uint16_t y)
    {
        uint16_t index = (y*DISPLAY_WIDTH+x)/2;
        return &buffer->buffer[index];
    };

#endif
