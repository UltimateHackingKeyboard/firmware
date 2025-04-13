/*******************************************************************************
 * Size: 12 px
 * Bpp: 4
 * Opts: --lv-font-name FontAwesome12 --format lvgl --bpp 4 -o font_awesome_12.c --size 12 --font /opt/fontawesome/otfs/font_awesome_6_regular.ttf --range 0xf057,0xf8dd,0xf1e6,0xe000,0xf071 --no-compress
 ******************************************************************************/

#include "font_awesome_12.h"
#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef FONTAWESOME12
#define FONTAWESOME12 1
#endif

#if FONTAWESOME12

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    // These have to follow order defined in font_awesome_12_data
    /* U+E000 "circle-xmark-large" */
    0x0, 0x0, 0x13, 0x31, 0x0, 0x0, 0x0, 0x3b,
    0xfe, 0xef, 0xb3, 0x0, 0x5, 0xf8, 0x10, 0x1,
    0x8f, 0x40, 0x2f, 0x53, 0x10, 0x1, 0x35, 0xf2,
    0x99, 0x8, 0xd1, 0x1d, 0x90, 0x99, 0xe4, 0x0,
    0xad, 0xda, 0x0, 0x4e, 0xf2, 0x0, 0x1f, 0xf1,
    0x0, 0x2f, 0xe4, 0x1, 0xda, 0xad, 0x10, 0x4e,
    0x99, 0xa, 0xa0, 0xa, 0xa0, 0x99, 0x2f, 0x40,
    0x0, 0x0, 0x5, 0xf2, 0x5, 0xf8, 0x10, 0x1,
    0x8f, 0x40, 0x0, 0x3b, 0xfe, 0xef, 0xb3, 0x0,
    0x0, 0x0, 0x13, 0x31, 0x0, 0x0,

    /* U+F057 "" */
    0x0, 0x0, 0x13, 0x31, 0x0, 0x0, 0x0, 0x2,
    0xbf, 0xee, 0xfb, 0x20, 0x0, 0x4, 0xf9, 0x10,
    0x1, 0x8f, 0x50, 0x1, 0xf5, 0x0, 0x0, 0x0,
    0x4f, 0x20, 0x8a, 0x0, 0x30, 0x3, 0x0, 0x99,
    0xd, 0x40, 0xd, 0x98, 0xe0, 0x3, 0xe0, 0xe3,
    0x0, 0x1f, 0xf2, 0x0, 0x2f, 0xd, 0x40, 0x8,
    0xed, 0x90, 0x3, 0xe0, 0x8a, 0x0, 0xa2, 0x1a,
    0x0, 0x99, 0x1, 0xe5, 0x0, 0x0, 0x0, 0x4f,
    0x20, 0x4, 0xf9, 0x10, 0x1, 0x8f, 0x50, 0x0,
    0x2, 0xbf, 0xee, 0xfb, 0x20, 0x0, 0x0, 0x0,
    0x13, 0x31, 0x0, 0x0, 0x0,

    /* U+F071 "" */
    0x0, 0x0, 0xb, 0xb0, 0x0, 0x0, 0x0, 0x0,
    0x9c, 0xc9, 0x0, 0x0, 0x0, 0x2, 0xf2, 0x2f,
    0x20, 0x0, 0x0, 0xb, 0x89, 0x38, 0xc0, 0x0,
    0x0, 0x5e, 0xc, 0x50, 0xe5, 0x0, 0x0, 0xe6,
    0xc, 0x50, 0x6e, 0x0, 0x8, 0xc0, 0x4, 0x10,
    0xc, 0x80, 0x2f, 0x30, 0x7, 0x20, 0x3, 0xf2,
    0xaa, 0x0, 0xc, 0x50, 0x0, 0xaa, 0xf5, 0x22,
    0x22, 0x22, 0x22, 0x5f, 0x7e, 0xff, 0xff, 0xff,
    0xff, 0xe7,

    /* U+F1E6 "" */
    0x0, 0x50, 0x0, 0x50, 0x0, 0x1f, 0x0, 0xf,
    0x10, 0x2, 0xf0, 0x0, 0xf2, 0x1, 0x24, 0x22,
    0x24, 0x21, 0xdf, 0xff, 0xff, 0xff, 0xd4, 0xe0,
    0x0, 0x0, 0xe4, 0x3e, 0x0, 0x0, 0xe, 0x31,
    0xf2, 0x0, 0x2, 0xf1, 0x9, 0xc2, 0x2, 0xc9,
    0x0, 0x9, 0xfe, 0xf9, 0x0, 0x0, 0x1, 0xf1,
    0x0, 0x0, 0x0, 0xf, 0x0, 0x0, 0x0, 0x0,
    0x60, 0x0, 0x0,

    /* U+F8DD "signal-stream" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa,
    0x60, 0x0, 0x0, 0x0, 0x1d, 0x10, 0x4e, 0x11,
    0x0, 0x0, 0x2, 0xa, 0xa0, 0xa8, 0xc, 0x60,
    0x0, 0xe, 0x42, 0xf0, 0xe3, 0x3f, 0x0, 0x52,
    0x7, 0xb0, 0xd4, 0xf2, 0x5c, 0x1, 0xf9, 0x4,
    0xd0, 0xc5, 0xe3, 0x3f, 0x0, 0x51, 0x7, 0xb0,
    0xd4, 0xa8, 0xc, 0x60, 0x0, 0xe, 0x42, 0xf0,
    0x4e, 0x11, 0x0, 0x0, 0x2, 0xa, 0xa0, 0xa,
    0x60, 0x0, 0x0, 0x0, 0x1d, 0x10, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0,

    /* U+E003 "plugs-connected" */
    0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0,
    0x0, 0x2, 0x0, 0xcb, 0x0, 0x0, 0x4, 0xef,
    0xfd, 0xc1, 0x0, 0x0, 0x5f, 0xff, 0xff, 0x40,
    0x0, 0x1, 0x3f, 0xff, 0xff, 0x90, 0x0, 0x4f,
    0x43, 0xff, 0xff, 0x90, 0x1, 0xff, 0xf4, 0x3f,
    0xff, 0x20, 0x7, 0xff, 0xff, 0x44, 0xf6, 0x0,
    0x8, 0xff, 0xff, 0xf4, 0x10, 0x0, 0x3, 0xff,
    0xff, 0xf7, 0x0, 0x0, 0xb, 0xee, 0xff, 0x60,
    0x0, 0x0, 0x9d, 0x10, 0x20, 0x0, 0x0, 0x0,
    0x11, 0x0, 0x0, 0x0, 0x0, 0x0,

    /* U+F06B "gift" */
    0x0, 0x3, 0x0, 0x2, 0x30, 0x0, 0x1, 0xee,
    0xe2, 0x5f, 0xec, 0x0, 0x7, 0xa0, 0x9b, 0xf4,
    0xd, 0x40, 0x18, 0xd4, 0x5f, 0xd4, 0x5f, 0x61,
    0xed, 0xdd, 0xdf, 0xed, 0xdd, 0xec, 0xf2, 0x0,
    0xe, 0x90, 0x0, 0x4d, 0xdf, 0xff, 0xff, 0xff,
    0xff, 0xfa, 0x4d, 0x22, 0x2e, 0xa2, 0x22, 0xf1,
    0x4d, 0x0, 0xe, 0x90, 0x0, 0xf1, 0x4d, 0x0,
    0xe, 0x90, 0x0, 0xf1, 0x3d, 0x0, 0xe, 0x90,
    0x0, 0xf1, 0x1e, 0xdd, 0xdf, 0xed, 0xde, 0xd0,
    0x1, 0x44, 0x44, 0x44, 0x44, 0x0,

    /* U+E422 "lock-a" */
    0x0, 0x0, 0x57, 0x30, 0x0, 0x0, 0x2, 0xef,
    0xff, 0x80, 0x0, 0x0, 0xce, 0x30, 0x8f, 0x40,
    0x0, 0xf, 0x70, 0x0, 0xf8, 0x0, 0x2, 0xf6,
    0x0, 0xe, 0xa0, 0x8, 0xff, 0xff, 0xff, 0xff,
    0xd2, 0xff, 0xff, 0xfb, 0xff, 0xff, 0x7f, 0xff,
    0xf8, 0x1f, 0xff, 0xf8, 0xff, 0xff, 0x39, 0x8f,
    0xff, 0x8f, 0xff, 0x81, 0x21, 0xff, 0xf8, 0xff,
    0xf4, 0xff, 0x99, 0xff, 0x8e, 0xff, 0xff, 0xff,
    0xff, 0xf6, 0x28, 0x99, 0x99, 0x99, 0x96, 0x0,

    /* U+E423 "lock-hashtag" */
    0x0, 0x1, 0x56, 0x30, 0x0, 0x0, 0x2, 0xef,
    0xff, 0x80, 0x0, 0x0, 0xce, 0x31, 0x9f, 0x30,
    0x0, 0xf, 0x70, 0x0, 0xf8, 0x0, 0x1, 0xf5,
    0x0, 0xe, 0x90, 0x8, 0xff, 0xff, 0xff, 0xff,
    0xd1, 0xff, 0xfd, 0x9f, 0x6f, 0xff, 0x6f, 0xfc,
    0x21, 0x30, 0x7f, 0xf7, 0xff, 0xfb, 0x7f, 0x3f,
    0xff, 0x7f, 0xfd, 0x53, 0x71, 0xaf, 0xf7, 0xff,
    0xfb, 0x7f, 0x4f, 0xff, 0x7c, 0xff, 0xff, 0xff,
    0xff, 0xf3, 0x4, 0x44, 0x44, 0x44, 0x42, 0x0,

    /* U+E004 bluetooth-signal */
    0x0, 0x0, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xeb, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0xed, 0xc1, 0x0, 0x0, 0x4b, 0x0,
    0x5, 0x0, 0xe3, 0xad, 0x10, 0x0, 0xd, 0x60,
    0xc, 0xc1, 0xe3, 0x8e, 0x20, 0x6c, 0x5, 0xd0,
    0x0, 0xae, 0xfd, 0xd2, 0x0, 0xf, 0x21, 0xf0,
    0x0, 0x9, 0xfd, 0x10, 0xdd, 0xc, 0x50, 0xf2,
    0x0, 0x4e, 0xff, 0x70, 0x77, 0xe, 0x31, 0xf1,
    0x6, 0xf6, 0xe6, 0xea, 0x0, 0x6c, 0x5, 0xd0,
    0xc, 0x30, 0xe3, 0x5f, 0x30, 0x11, 0xc, 0x70,
    0x0, 0x0, 0xe8, 0xf6, 0x0, 0x0, 0x4c, 0x0,
    0x0, 0x0, 0xef, 0x40, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x93, 0x0, 0x0, 0x0, 0x0, 0x0,

    /* U+E004 bluetooth-signal-plus */
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xeb,
    0x00, 0x00, 0x00, 0x00, 0x0f, 0xff, 0x00, 0x00, 0xed, 0xc1, 0x00, 0x00,
    0x4b, 0x00, 0xf0, 0x05, 0x00, 0xe3, 0xad, 0x10, 0x00, 0x0d, 0x60, 0x00,
    0x0c, 0xc1, 0xe3, 0x8e, 0x20, 0x6c, 0x05, 0xd0, 0x00, 0x00, 0xae, 0xfd,
    0xd2, 0x00, 0x0f, 0x21, 0xf0, 0x00, 0x00, 0x09, 0xfd, 0x10, 0xdd, 0x0c,
    0x50, 0xf2, 0x00, 0x00, 0x4e, 0xff, 0x70, 0x77, 0x0e, 0x31, 0xf1, 0x00,
    0x06, 0xf6, 0xe6, 0xea, 0x00, 0x6c, 0x05, 0xd0, 0x00, 0x0c, 0x30, 0xe3,
    0x5f, 0x30, 0x11, 0x0c, 0x70, 0x00, 0x00, 0x00, 0xe8, 0xf6, 0x00, 0x00,
    0x4c, 0x00, 0x00, 0x00, 0x00, 0xef, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x93, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* U+F293 bluetooth */
    0x0, 0x0, 0x60, 0x0, 0x0, 0x0, 0xf, 0xc1,
    0x0, 0x0, 0x0, 0xfb, 0xd2, 0x1, 0x90, 0xf,
    0x19, 0xe1, 0xb, 0xd3, 0xf3, 0xdb, 0x0, 0x9,
    0xff, 0xf9, 0x0, 0x0, 0xb, 0xfb, 0x0, 0x0,
    0x9, 0xff, 0xf9, 0x0, 0xb, 0xd3, 0xf3, 0xdc,
    0x1, 0x80, 0xf, 0x19, 0xe1, 0x0, 0x0, 0xfb,
    0xd2, 0x0, 0x0, 0xf, 0xc1, 0x0, 0x0, 0x0,
    0x60, 0x0, 0x0,

    /* U+E001 "lock-square-a" hand tweaked */
    0x00, 0x66, 0x66, 0x66, 0x66, 0x00,
    0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0,
    0xf6, 0x00, 0x00, 0x00, 0x00, 0x6f,
    0xf6, 0x00, 0x09, 0xb0, 0x00, 0x6f,
    0xf6, 0x00, 0x1f, 0xf1, 0x00, 0x6f,
    0xf6, 0x00, 0x9d, 0xb9, 0x00, 0x6f,
    0xf6, 0x02, 0xf4, 0x4f, 0x20, 0x6f,
    0xf6, 0x0c, 0xff, 0xff, 0xc0, 0x6f,
    0xf6, 0x1e, 0xa5, 0x5a, 0xe1, 0x6f,
    0xf6, 0x5c, 0x00, 0x00, 0xc5, 0x6f,
    0xf6, 0x00, 0x00, 0x00, 0x00, 0x6f,
    0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0,
    0x00, 0x66, 0x66, 0x66, 0x66, 0x00,

    /* U+E002 "lock-square-one" hand tweaked */
    0x00, 0x66, 0x66, 0x66, 0x66, 0x00,
    0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0,
    0xf6, 0x00, 0x00, 0x00, 0x00, 0x6f,
    0xf6, 0x00, 0x04, 0xf2, 0x00, 0x6f,
    0xf6, 0x00, 0x5f, 0xf3, 0x00, 0x6f,
    0xf6, 0x05, 0xfb, 0xf3, 0x00, 0x6f,
    0xf6, 0x00, 0x22, 0xf3, 0x00, 0x6f,
    0xf6, 0x00, 0x02, 0xf3, 0x00, 0x6f,
    0xf6, 0x00, 0x02, 0xf3, 0x00, 0x6f,
    0xf6, 0x00, 0x01, 0xe1, 0x00, 0x6f,
    0xf6, 0x00, 0x00, 0x00, 0x00, 0x6f,
    0x0f, 0xff, 0xff, 0xff, 0xff, 0xf0,
    0x00, 0x66, 0x66, 0x66, 0x66, 0x00,

    /* U+E0B1 "battery-low" */
  0x01, 0x22, 0x22, 0x22, 0x21, 0x00, 0x07, 0xff, 0xff, 0xff, 0xff, 0xf7,
  0x00, 0xf4, 0x00, 0x00, 0x00, 0x05, 0xe0, 0x0f, 0x2e, 0x90, 0x00, 0x00,
  0x3f, 0x64, 0xf2, 0xe9, 0x00, 0x00, 0x03, 0xff, 0x6f, 0x2e, 0x90, 0x00,
  0x00, 0x3f, 0x64, 0xf4, 0x00, 0x00, 0x00, 0x05, 0xe0, 0x07, 0xff, 0xff,
  0xff, 0xff, 0xf7, 0x00, 0x01, 0x22, 0x22, 0x22, 0x21, 0x00, 0x00,

  /* U+E006 "battery-exclamation-vertical" */
  0x02, 0xf2, 0x06, 0xff, 0xf6, 0xf0, 0x00, 0xff, 0x0f, 0x0f, 0xf0, 0xf0,
  0xff, 0x0f, 0x0f, 0xf0, 0x00, 0xff, 0x0f, 0x0f, 0xf0, 0x00, 0xf6, 0xff,
  0xf6,


  /* U+E009 "bolt-solid-narrow */
  0x0, 0x0, 0x38, 0x0, 0x0, 0x1f, 0xa0, 0x0,
  0xf, 0xf4, 0x0, 0xb, 0xff, 0x0, 0x7, 0xff,
  0xe6, 0x20, 0xdf, 0xff, 0xf6, 0x0, 0xf, 0xfc,
  0x0, 0x5, 0xff, 0x10, 0x0, 0xbf, 0x30, 0x0,
  0xf, 0x60, 0x0, 0x0, 0x20, 0x0, 0x0,


  /* "percent" */
  0x4b, 0x40, 0x67, 0xf0, 0xf1, 0xc0, 0x79, 0x7b, 0x30, 0x07, 0x5a, 0x00,
  0x00, 0x88, 0x00, 0x00, 0xa5, 0x70, 0x03, 0xb7, 0x97, 0x0c, 0x4f, 0x0f,
  0x76, 0x04, 0xb4
};

/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

#define COMPUTE_GLYPH_LEN(BOXW, BOXH) (((BOXW)*(BOXH)+1)/2)
const uint8_t GlyphOffset_0 = 0;
#define GLYPH_DATA(INDEX0, INDEX1, NAME, ADVW, BOXW, BOXH, OFSX, OFSY) const uint16_t GlyphOffset_##INDEX1 = GlyphOffset_##INDEX0 + COMPUTE_GLYPH_LEN(BOXW, BOXH);
#include "font_awesome_12_data.h"
#undef GLYPH_DATA

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,

#define GLYPH_DATA(INDEX0, INDEX1, NAME, ADVW, BOXW, BOXH, OFSX, OFSY) {.bitmap_index = GlyphOffset_##INDEX0, .adv_w = ADVW, .box_w = BOXW, .box_h = BOXH, .ofs_x = OFSX, .ofs_y = OFSY},
#include "font_awesome_12_data.h"
#undef GLYPH_DATA
};


/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/

// unused I believe
static const uint16_t unicode_list_0[] = {
    0x0, 0x1057, 0x1071, 0x11e6, 0x18dd
};

/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 57344, .range_length = 6366, .glyph_id_start = 1,
        .unicode_list = unicode_list_0, .glyph_id_ofs_list = NULL, .list_length = 5, .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }
};

/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
    static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
        .glyph_bitmap = glyph_bitmap,
        .glyph_dsc = glyph_dsc,
        .cmaps = cmaps,
        .kern_dsc = NULL,
        .kern_scale = 0,
        .cmap_num = 1,
        .bpp = 4,
        .kern_classes = 0,
        .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
        .cache = &cache
#endif
    };


    /*-----------------
     *  PUBLIC FONT
     *----------------*/

    /*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
    const lv_font_t FontAwesome12 = {
#else
        lv_font_t FontAwesome12 = {
#endif
            .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
            .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
            .line_height = 13,          /*The maximum line height required by the font*/
            .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
            .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
            .underline_position = 0,
            .underline_thickness = 0,
#endif
            .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
            .fallback = NULL,
            .user_data = NULL
        };



#endif /*#if FONTAWESOME12*/

