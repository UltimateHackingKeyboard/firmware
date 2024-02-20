#include "oled_text_renderer.h"
#include "framebuffer.h"
#include "lvgl/lvgl.h"
#include "keyboard/logger.h"
#include "fonts/fonts.h"
#include "oled_buffer.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "widgets/widget.h"

#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))

static uint8_t drawGlyph(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, uint8_t glyphIdx)
{
    int16_t canvasOffsetX = canvas == NULL ? 0 : canvas->x;
    int16_t canvasOffsetY = canvas == NULL ? 0 : canvas->y;
    int16_t canvasWidth = canvas == NULL ? buffer->width : canvas->w;
    int16_t canvasHeight = canvas == NULL ? buffer->height : canvas->h;

   const uint8_t* bitmap = font->dsc->glyph_bitmap;
   const lv_font_fmt_txt_glyph_dsc_t* glyph = &font->dsc->glyph_dsc[glyphIdx];

   int16_t rw = glyph->adv_w/16;
   int16_t rh = font->line_height;

   // clear the area. Glyph bitmaps generally code just the non-empty bounding box, so full clearing is necessary
   for (uint16_t i = MAX(y,0); i < MIN(canvasHeight, y+rh); i++) {
       for (uint16_t j = MAX(x,0); j < MIN(canvasWidth, x+rw); j++) {
           Framebuffer_SetPixel(buffer, canvasOffsetX+j, canvasOffsetY+i, 0);
       }
   }

   int16_t w = glyph->box_w;
   int16_t h = glyph->box_h;
   int16_t top = font->line_height - font->base_line - h - glyph->ofs_y;

   // draw the glyph
   for (uint16_t iy = 0; iy < h; iy++) {
       int16_t dstY = top+y+iy;
       if (dstY < 0 || dstY >= canvasHeight) {
           continue;
       }
       for (uint16_t ix = 0; ix < w; ix++) {
           int16_t dstX = glyph->ofs_x+x+ix;

           if (dstX < 0 || dstX >= canvasWidth) {
               continue;
           }

           uint16_t pixelIndex = iy * w + ix;
           uint8_t byte = bitmap[glyph->bitmap_index + pixelIndex/2];
           uint8_t pixelValue;
           if (pixelIndex % 2 == 0) {
               pixelValue = byte & 0xf0;
           } else {
               pixelValue = byte << 4;
           }

           Framebuffer_SetPixel(buffer, canvasOffsetX + dstX, canvasOffsetY + dstY, pixelValue);
       }
   }

   return rw;
}

void Framebuffer_DrawText(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, const char* text)
{
    uint16_t consumed = 0;
    while (*text != '\0') {
        consumed += drawGlyph(canvas, buffer, x+consumed, y, font, (*text)-31);
        text++;
    }
}

static uint16_t approximateTextLength(const lv_font_t* font, const char* text)
{
    //assume monospace font!
    const lv_font_fmt_txt_glyph_dsc_t* someGlyph = &font->dsc->glyph_dsc[1];
    uint8_t charWidth = someGlyph->adv_w/16;
    return strlen(text) * charWidth;
}

void Framebuffer_DrawTextAnchored(widget_t* canvas, framebuffer_t* buffer, anchor_type_t horizontalAnchor, anchor_type_t verticalAnchor, const lv_font_t* font, const char* text)
{
    int16_t canvasWidth = canvas == NULL ? buffer->width : canvas->w;
    int16_t canvasHeight = canvas == NULL ? buffer->height : canvas->h;

    int16_t x, y;

    switch (horizontalAnchor) {
        default:
        case AnchorType_Begin:
            x = 0;
            break;
        case AnchorType_Center:
            x = canvasWidth/2 - approximateTextLength(font, text)/2;
            break;
        case AnchorType_End:
            x = canvasWidth - approximateTextLength(font, text);
            break;
    }

    switch (verticalAnchor) {
        default:
        case AnchorType_Begin:
            y = 0;
            break;
        case AnchorType_Center:
            y = canvasHeight/2 - font->line_height/2;
            break;
        case AnchorType_End:
            y = canvasHeight - font->line_height;
            break;
    }

    Framebuffer_DrawText(canvas, buffer, x, y, font, text);
}
