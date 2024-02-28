#include "lvgl.h"

bool lv_font_get_glyph_dsc_fmt_txt(const lv_font_t * font, lv_font_glyph_dsc_t * dsc_out, uint32_t unicode_letter, uint32_t unicode_letter_next)
{
    return true;
}

const void * lv_font_get_bitmap_fmt_txt(lv_font_glyph_dsc_t * g_dsc, uint32_t unicode_letter,
                                        lv_draw_buf_t * draw_buf)
{
    return NULL;
}
