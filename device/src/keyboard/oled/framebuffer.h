#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>

// Macros:

// Typedefs:

    typedef enum {
        AnchorType_Begin,
        AnchorType_Center,
        AnchorType_End,
    } anchor_type_t;

    typedef struct {
        uint16_t width;
        uint16_t height;
        uint8_t buffer[];
    } framebuffer_t;

    typedef struct widget_t widget_t;

// Variables:

    static const uint8_t Framebuffer_PixelIsDirty = 0x01;

// Functions:

    void Framebuffer_Clear(widget_t* canvas, framebuffer_t* buffer);
    void Framebuffer_DrawHLine(widget_t* canvas, framebuffer_t* buffer, uint8_t x1, uint8_t x2, uint8_t y);
    void Framebuffer_DrawVLine(widget_t* canvas, framebuffer_t* buffer, uint8_t x, uint8_t y1, uint8_t y2);

    static inline void Framebuffer_SetPixel(framebuffer_t* buffer, uint16_t x, uint16_t y, uint8_t value)
    {
        uint16_t index = y*buffer->width+x;
        if ((buffer->buffer[index] & 0xf0) != (value & 0xf0)) {
            buffer->buffer[index] = (value & 0xf0) | Framebuffer_PixelIsDirty;
        }
    };


#endif