#ifndef __FONT_AWESOME_12_H__
#define __FONT_AWESOME_12_H__


// Typedefs:

/**
 * Instructions to add a new glyph:
 * - Generate a .c lv font file using instructions in fonts.h"
 * - Add the glyph's data into the glyph_bitmap array in font_awesome_12.c
 * - Add the metadata into font_awesome_12_data.h
 * */

    typedef enum {
        #define GLYPH_DATA(INDEX0, INDEX1, NAME, ADVW, BOXW, BOXH, OFSX, OFSY) FontIcon_##NAME = 32+INDEX0,
        #include "font_awesome_12_data.h"
        #undef GLYPH_DATA
        FontIcon_AfterLast,
        FontIcon_Count = FontIcon_AfterLast - 32,
    } font_icons_t;

#endif
