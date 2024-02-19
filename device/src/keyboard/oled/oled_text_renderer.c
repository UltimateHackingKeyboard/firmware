#include "oled_text_renderer.h"
#include "framebuffer.h"
#include "lvgl/lvgl.h"
#include "keyboard/logger.h"
#include "fonts/fonts.h"
#include "oled_buffer.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "widgets/widget.h"

static uint8_t drawGlyph(widget_t* canvas, framebuffer_t* buffer, uint16_t x, uint16_t y, const lv_font_t* font, uint8_t glyphIdx)
{
    uint16_t canvasOffsetX = canvas == NULL ? 0 : canvas->x;
    uint16_t canvasOffsetY = canvas == NULL ? 0 : canvas->y;
    uint16_t canvasWidth = canvas == NULL ? buffer->width : canvas->w;
    uint16_t canvasHeight = canvas == NULL ? buffer->height : canvas->h;

   const uint8_t* bitmap = font->dsc->glyph_bitmap;
   const lv_font_fmt_txt_glyph_dsc_t* glyph = &font->dsc->glyph_dsc[glyphIdx];

   uint16_t rw = glyph->adv_w/16;
   uint16_t rh = font->line_height;

   for (uint16_t i = 0; i < rh; i++) {
       for (uint16_t j = 0; j < rw; j++) {
           if (x+j < canvasWidth && y+i < canvasHeight) {
               Framebuffer_SetPixel(buffer, canvasOffsetX+x+j, canvasOffsetY+y+i, 0);
           }
       }
   }

   uint16_t w = glyph->box_w;
   uint16_t h = glyph->box_h;
   uint16_t top = font->line_height - font->base_line - h - glyph->ofs_y;

   for (uint16_t iy = 0; iy < h; iy++) {
       for (uint16_t ix = 0; ix < w; ix++) {
           uint16_t pixelIndex = iy * w + ix;
           uint8_t byte = bitmap[glyph->bitmap_index + pixelIndex/2];
           uint8_t pixelValue;
           if (pixelIndex % 2 == 0) {
               pixelValue = byte & 0xf0;
           } else {
               pixelValue = byte << 4;
           }
           uint16_t dstX = glyph->ofs_x+x+ix;
           uint16_t dstY = top+y+iy;
           if (dstX < canvasWidth && dstY < canvasHeight) {
               Framebuffer_SetPixel(buffer, canvasOffsetX + dstX, canvasOffsetY + dstY, pixelValue);
           }
       }
   }

   return rw;
}

void Framebuffer_DrawText(widget_t* canvas, framebuffer_t* buffer, uint16_t x, uint16_t y, const lv_font_t* font, const char* text)
{
    uint16_t consumed = 0;
    while (*text != '\0') {
        consumed += drawGlyph(canvas, buffer, x+consumed, y, font, (*text)-31);
        text++;
    }
    buffer->dirty = true;
}

