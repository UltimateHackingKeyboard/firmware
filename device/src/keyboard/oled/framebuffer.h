#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

// Includes:

#include <inttypes.h>
#include <stdbool.h>

// Macros:

// Typedefs:

typedef struct {
    uint16_t width;
    uint16_t height;
    bool dirty;
    uint8_t buffer[];
} framebuffer_t;


// Variables:

// Functions:

void Framebuffer_Shift(framebuffer_t* buffer, uint16_t shiftBy);

static inline void Framebuffer_SetPixel(framebuffer_t* buffer, uint16_t x, uint16_t y, uint8_t value)
{
    uint16_t index = y*buffer->width+x;
    if ((buffer->buffer[index] & 0xf0) != (value & 0xf0) && x < buffer->width && y < buffer->height) {
        buffer->buffer[index] = (value & 0xf0) | 0x01;
    }
};


#endif
