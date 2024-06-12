#include "oled_text_renderer.h"
#include "framebuffer.h"
#include "lvgl/lvgl.h"
#include "fonts/fonts.h"
#include "oled_buffer.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "widgets/widget.h"
#include <zephyr/sys/util.h>
#include "legacy/str_utils.h"

static string_segment_t truncateText(const lv_font_t* font, uint8_t width, const char* text, const char* textEnd)
{
    const lv_font_fmt_txt_glyph_dsc_t* someGlyph = &font->dsc->glyph_dsc[1];
    uint8_t charWidth = someGlyph->adv_w/16;
    uint8_t maxLen = width/charWidth;
    uint8_t len = textEnd == NULL ? strlen(text) : textEnd - text;
    uint8_t truncatedLen = MIN(maxLen, len);
    return (string_segment_t){ .start = text, .end = text + truncatedLen };
}

static int16_t computeAlignment(int16_t width, int16_t objectWidth, int16_t alignment)
{
    int16_t offset = -((-alignment & ((1 << 9) - 1)) - 256);

    switch ((-alignment) >> 9) {
        default:
        case AlignmentType_Begin_:
            return offset;
        case AlignmentType_Center_:
            return width/2 - objectWidth/2 + offset;
        case AlignmentType_End_:
            return width - objectWidth + offset;
    }
}


static uint8_t drawGlyph(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, uint8_t glyphIdx, bool gray)
{
    int16_t canvasOffsetX = canvas == NULL ? 0 : canvas->x;
    int16_t canvasOffsetY = canvas == NULL ? 0 : canvas->y;
    int16_t canvasWidth = canvas == NULL ? DISPLAY_WIDTH : canvas->w;
    int16_t canvasHeight = canvas == NULL ? DISPLAY_HEIGHT : canvas->h;

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

           if (gray) {
               pixelValue /= 8;
           }

           Framebuffer_SetPixel(buffer, canvasOffsetX + dstX, canvasOffsetY + dstY, pixelValue);
       }
   }

   return rw;
}

// TODO: return resulting bounds rectangle?
void Framebuffer_DrawText(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, const char* text, const char* textEnd)
{
    bool gray = false;

    if (x < 0 || y < 0) {
        int16_t canvasWidth = canvas == NULL ? DISPLAY_WIDTH : canvas->w;
        int16_t canvasHeight = canvas == NULL ? DISPLAY_HEIGHT : canvas->h;
        string_segment_t truncatedText = truncateText(font, canvasWidth, text, textEnd);
        textEnd = truncatedText.end;
        int16_t textWidth = Framebuffer_TextWidth(font, text, textEnd);
        int16_t textHeight = font->line_height;

        if (x < 0) {
            x = computeAlignment(canvasWidth, textWidth, x);
        }

        if (y < 0) {
            y = computeAlignment(canvasHeight, textHeight, y);
        }
    }

    uint16_t consumed = 0;
    while (*text != '\0' && (textEnd == NULL || text < textEnd)) {
        if (*text >= 32) {
            consumed += drawGlyph(canvas, buffer, x+consumed, y, font, (*text)-31, gray);
        } else {
            switch (*text) {
                case FontControl_WhiteText:
                    gray = false;
                    break;
                case FontControl_GrayText:
                    gray = true;
                    break;
            }
        }
        text++;
    }
}

uint16_t Framebuffer_TextWidth(const lv_font_t* font, const char* text, const char* textEnd)
{
    int len = 0;
    for(const char *ptr = text; ptr != textEnd && *ptr != 0; ptr++) {
        if(*ptr >= 32) {
            len++;
        }
    }

    const lv_font_fmt_txt_glyph_dsc_t* someGlyph = &font->dsc->glyph_dsc[1];
    uint8_t charWidth = someGlyph->adv_w/16;
    return len * charWidth;
}

