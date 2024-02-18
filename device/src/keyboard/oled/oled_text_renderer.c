#include "oled_text_renderer.h"
#include "lvgl/lvgl.h"
#include "fonts/fonts.h"
#include "oled_buffer.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static uint8_t drawGlyph(uint16_t x, uint16_t y, const lv_font_t* font, uint8_t glyphIdx)
{
   const uint8_t* bitmap = font->dsc->glyph_bitmap;
   const lv_font_fmt_txt_glyph_dsc_t* glyph = &font->dsc->glyph_dsc[glyphIdx];

   uint16_t rw = glyph->adv_w/16;
   uint16_t rh = font->line_height;

   for (uint16_t i = 0; i < rh; i++) {
       for (uint16_t j = 0; j < rw; j++) {
           OledBuffer_SetPixel(x+j, y+i, 0);
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
           OledBuffer_SetPixel(dstX, dstY, pixelValue);
       }
   }

   return rw;
}

void Oled_DrawText(uint16_t x, uint16_t y, const lv_font_t* font, const char* text)
{
    uint16_t consumed = 0;
    while (*text != '\0') {
        consumed += drawGlyph(x+consumed, y, font, (*text)-31);
        text++;
    }
    OledBuffer_NeedsRedraw = true;
}

void Oled_LogConstant(const char* text)
{
    const lv_font_t* logFont = &CustomMono8;
    uint8_t line_height = logFont->line_height;
    OledBuffer_Shift(line_height);
    Oled_DrawText(0, DISPLAY_HEIGHT-line_height, logFont, text);
    OledBuffer_NeedsRedraw = true;
}

void Oled_Log(const char *fmt, ...)
{
    va_list myargs;
    va_start(myargs, fmt);
    char buffer[256];
    vsprintf(buffer, fmt, myargs);
    Oled_LogConstant(buffer);
    OledBuffer_NeedsRedraw = true;
}

