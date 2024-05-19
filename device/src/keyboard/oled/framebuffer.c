#include "framebuffer.h"
#include "widgets/console_widget.h"
#include "widgets/widget.h"
#include <stdint.h>
#include <stdlib.h>
#include <zephyr/sys/util.h>

void Framebuffer_Clear(widget_t* canvas, framebuffer_t* buffer)
{
    uint16_t canvasOffsetX = canvas == NULL ? 0 : canvas->x;
    uint16_t canvasOffsetY = canvas == NULL ? 0 : canvas->y;
    uint16_t canvasWidth = canvas == NULL ? DISPLAY_WIDTH : canvas->w;
    uint16_t canvasHeight = canvas == NULL ? DISPLAY_HEIGHT : canvas->h;


    // TODO: optimize this
    for (uint16_t y = 0; y < canvasHeight; y++) {
        for (uint16_t x = 0; x < canvasWidth; x++) {
            Framebuffer_SetPixel(buffer, canvasOffsetX+x, canvasOffsetY+y, 0);
        }
        buffer->dirtyRanges[canvasOffsetY+y].min = MIN(buffer->dirtyRanges[y].min, canvasOffsetX);
        buffer->dirtyRanges[canvasOffsetY+y].max = MAX(buffer->dirtyRanges[y].max, canvasOffsetX+canvasWidth-1);
    }
}

void Framebuffer_DrawHLine(widget_t* canvas, framebuffer_t* buffer, uint8_t x1, uint8_t x2, uint8_t y)
{
    uint16_t canvasOffsetX = canvas == NULL ? 0 : canvas->x;
    uint16_t canvasOffsetY = canvas == NULL ? 0 : canvas->y;
    uint16_t canvasWidth = canvas == NULL ? DISPLAY_WIDTH : canvas->w;
    uint16_t canvasHeight = canvas == NULL ? DISPLAY_HEIGHT : canvas->h;

    if (x1 > x2) {
        uint8_t tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    if (y < canvasHeight) {
        for (uint16_t x = x1; x < x2 && x < canvasWidth; x++) {
            Framebuffer_SetPixel(buffer, canvasOffsetX+x, canvasOffsetY+y, 0xff);
        }
        buffer->dirtyRanges[canvasOffsetY+y].min = MIN(buffer->dirtyRanges[y].min, canvasOffsetX+x1);
        buffer->dirtyRanges[canvasOffsetY+y].max = MAX(buffer->dirtyRanges[y].max, canvasOffsetX+x2);
    }
}

void Framebuffer_DrawVLine(widget_t* canvas, framebuffer_t* buffer, uint8_t x, uint8_t y1, uint8_t y2)
{
    uint16_t canvasOffsetX = canvas == NULL ? 0 : canvas->x;
    uint16_t canvasOffsetY = canvas == NULL ? 0 : canvas->y;
    uint16_t canvasWidth = canvas == NULL ? DISPLAY_WIDTH : canvas->w;
    uint16_t canvasHeight = canvas == NULL ? DISPLAY_HEIGHT : canvas->h;

    if (y1 > y2) {
        uint8_t tmp = y1;
        y1 = y2;
        y2 = tmp;
    }

    if (x < canvasWidth) {
        for (uint16_t y = y1; y < y2 && y < canvasHeight; y++) {
            Framebuffer_SetPixel(buffer, canvasOffsetX+x, canvasOffsetY+y, 0xff);
            buffer->dirtyRanges[canvasOffsetY+y].min = MIN(buffer->dirtyRanges[y].min, canvasOffsetX+x);
            buffer->dirtyRanges[canvasOffsetY+y].max = MAX(buffer->dirtyRanges[y].max, canvasOffsetX+x);
        }
    }
}

