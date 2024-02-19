#include "framebuffer.h"

void Framebuffer_Shift(framebuffer_t* buffer, uint16_t shiftBy)
{
    if (shiftBy > buffer->height) {
        shiftBy = buffer->height;
    }

    for (uint16_t y = 0; y < buffer->height-shiftBy; y++) {
        for (uint16_t x = 0; x < buffer->width; x++) {
            Framebuffer_SetPixel(buffer, x, y, buffer->buffer[y*buffer->width+shiftBy*buffer->width+x]);
        }
    }
    for (uint16_t y = buffer->height - shiftBy; y < buffer->height; y++) {
        for (uint16_t x = 0; x < buffer->width; x++) {
            Framebuffer_SetPixel(buffer, x, y, 0);
        }
    }
}

