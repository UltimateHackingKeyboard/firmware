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
#include <sys/_stdint.h>
#include <zephyr/sys/util.h>
#include "str_utils.h"

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


static uint8_t drawGlyph(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, uint8_t glyphIdx, uint8_t color)
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

           if (color == FontControl_NextCharGray) {
               pixelValue /= 8;
           }
           if (color == FontControl_NextCharBlack) {
               pixelValue = 0;
           }

           Framebuffer_SetPixel(buffer, canvasOffsetX + dstX, canvasOffsetY + dstY, pixelValue);
       }
   }

   return rw;
}

static uint8_t getUtf8Length(uint8_t firstByte) {
    if ((firstByte & 0xe0) == 0xc0) {
        return 2;
    } else if ((firstByte & 0xf0) == 0xe0) {
        return 3;
    } else if ((firstByte & 0xf8) == 0xf0) {
        return 4;
    } else {
        return 1;
    }
}

// TODO: return resulting bounds rectangle?
void Framebuffer_DrawText(widget_t* canvas, framebuffer_t* buffer, int16_t x, int16_t y, const lv_font_t* font, const char* text, const char* textEnd)
{
    bool truncated;
    if (x < 0 || y < 0) {
        int16_t canvasWidth = canvas == NULL ? DISPLAY_WIDTH : canvas->w;
        int16_t canvasHeight = canvas == NULL ? DISPLAY_HEIGHT : canvas->h;
        const char* truncatedEnd;
        int16_t textWidth = Framebuffer_TextWidth(font, text, textEnd, canvasWidth, &truncated, &truncatedEnd);
        int16_t textHeight = font->line_height;

        if (truncated) {
            textEnd = truncatedEnd;
        }

        if (x < 0) {
            x = computeAlignment(canvasWidth, textWidth, x);
        }

        if (y < 0) {
            y = computeAlignment(canvasHeight, textHeight, y);
        }
    }

    uint8_t color = FontControl_NextCharWhite;
    bool icon12 = false;

    uint16_t consumed = 0;
    while (*text != '\0' && (textEnd == NULL || text < textEnd)) {
        if (*text > 127) {
            consumed += drawGlyph(canvas, buffer, x+consumed, y, font, '*'-31, color);
            icon12 = false;
            color = FontControl_NextCharWhite;
            text+= getUtf8Length(*text);
        } else if (*text >= 32) {
            consumed += drawGlyph(canvas, buffer, x+consumed, y, icon12 ? &FontAwesome12 : font, (*text)-31, color);
            icon12 = false;
            color = FontControl_NextCharWhite;
            text++;
        } else if (*text < 32) {
            switch (*text) {
                case FontControl_NextCharBlack:
                case FontControl_NextCharGray:
                case FontControl_NextCharWhite:
                    color = *text;
                    break;
                case FontControl_NextCharIcon12:
                    icon12 = true;
                    break;
            }
            text++;
        }
    }

    if (truncated) {
        for (uint8_t i = 0; i < 3; i++) {
            consumed += drawGlyph(canvas, buffer, x+consumed, y, font, '.'-31, color);
        }
    }
}

static uint16_t getGlyphWidth(const lv_font_t* font, uint8_t glyphIdx)
{
    const lv_font_fmt_txt_glyph_dsc_t* someGlyph = &font->dsc->glyph_dsc[glyphIdx];
    return someGlyph->adv_w/16;
}

uint16_t Framebuffer_GetGlyphWidth(const lv_font_t* font, uint8_t glyphIdx)
{
    return getGlyphWidth(font, glyphIdx-31);
}

uint16_t Framebuffer_TextWidth(const lv_font_t* font, const char* text, const char* textEnd, uint16_t maxWidth, bool* truncated, const char** truncatedText)
{
    bool dummyBool;
    const char* dummyChar;

    if (truncated == NULL) {
        truncated = &dummyBool;
    }

    if (truncatedText == NULL) {
        truncatedText = &dummyChar;
    }

    uint16_t dotsWidth = getGlyphWidth(font, '.'-31)*3;

    uint8_t color = FontControl_NextCharWhite;
    bool icon12 = false;

    *truncatedText = text;

    uint16_t previousConsumed = 0;
    uint16_t consumed = 0;
    while (*text != '\0' && (textEnd == NULL || text < textEnd)) {
        previousConsumed = consumed;
        if (*text > 127) {
            consumed += getGlyphWidth(font, '*'-31);
            icon12 = false;
            color = FontControl_NextCharWhite;
            text+= getUtf8Length(*text);
        } else if (*text >= 32) {
            consumed += getGlyphWidth(icon12 ? &FontAwesome12 : font, (*text)-31);
            icon12 = false;
            color = FontControl_NextCharWhite;
            text++;
        } else if (*text < 32) {
            switch (*text) {
                case FontControl_NextCharBlack:
                case FontControl_NextCharGray:
                case FontControl_NextCharWhite:
                    color = *text;
                    break;
                case FontControl_NextCharIcon12:
                    icon12 = true;
                    break;
            }
            text++;
        }

        if ( consumed + dotsWidth < maxWidth ) {
            *truncatedText = text;
        } else {
            *truncated = true;
            return previousConsumed + dotsWidth;
        }
    }

    *truncated = false;
    return consumed;
}
