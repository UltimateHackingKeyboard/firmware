/*******************************************************************************
 * Size: 8 px
 * Bpp: 4
 * Opts: 
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef JETBRAINSMONO8
#define JETBRAINSMONO8 1
#endif

#if JETBRAINSMONO8

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0020 " " */

    /* U+0021 "!" */
    0xd, 0xc, 0xb, 0xb, 0x6, 0x0, 0x1d,

    /* U+0022 "\"" */
    0xc2, 0xac, 0x29, 0x71, 0x60,

    /* U+0023 "#" */
    0x5, 0x37, 0x10, 0x71, 0x90, 0x3c, 0x9c, 0x40,
    0x80, 0x80, 0x6c, 0x9b, 0x22, 0x64, 0x40, 0x44,
    0x62, 0x0,

    /* U+0024 "$" */
    0x0, 0x70, 0x0, 0x7d, 0x60, 0x39, 0x8b, 0x12,
    0xb8, 0x0, 0x6, 0xe6, 0x0, 0x7, 0xb1, 0x58,
    0x7a, 0x20, 0x9d, 0x70, 0x0, 0x70, 0x0,

    /* U+0025 "%" */
    0x8b, 0x13, 0x69, 0x45, 0x90, 0x94, 0xa4, 0x6,
    0xa9, 0x0, 0x7, 0x5a, 0x41, 0x88, 0x19, 0x81,
    0x5a, 0x60,

    /* U+0026 "&" */
    0xa, 0xb4, 0x1, 0xa1, 0x80, 0xc, 0x10, 0x4,
    0xc9, 0x3, 0xb0, 0x8b, 0x4b, 0x4, 0xe0, 0x5c,
    0xa3, 0x90,

    /* U+0027 "'" */
    0xc, 0xc, 0x7,

    /* U+0028 "(" */
    0x0, 0x40, 0xa7, 0x39, 0x7, 0x40, 0x83, 0x8,
    0x30, 0x65, 0x1, 0xc0, 0x4, 0xb0, 0x0,

    /* U+0029 ")" */
    0x3, 0x0, 0xa, 0x70, 0x0, 0xb1, 0x0, 0x74,
    0x0, 0x65, 0x0, 0x64, 0x0, 0x83, 0x1, 0xc0,
    0xc, 0x20, 0x0, 0x0,

    /* U+002A "*" */
    0x0, 0xa0, 0x6, 0x5a, 0x64, 0x17, 0xf5, 0x0,
    0xa6, 0x80, 0x4, 0x5, 0x0,

    /* U+002B "+" */
    0x0, 0xb0, 0x0, 0xb, 0x0, 0x5b, 0xeb, 0x30,
    0xb, 0x0, 0x0, 0x20, 0x0,

    /* U+002C "," */
    0x9, 0x1b, 0x38,

    /* U+002D "-" */
    0x9b, 0x70,

    /* U+002E "." */
    0x5, 0x2, 0xe0,

    /* U+002F "/" */
    0x0, 0x5, 0x0, 0x0, 0xc0, 0x0, 0x38, 0x0,
    0x9, 0x30, 0x0, 0xc0, 0x0, 0x38, 0x0, 0x9,
    0x30, 0x0, 0xc0, 0x0, 0x38, 0x0, 0x0,

    /* U+0030 "0" */
    0xa, 0xb8, 0x4, 0x70, 0xa1, 0x55, 0x8, 0x25,
    0x5a, 0x82, 0x55, 0x8, 0x24, 0x70, 0xa0, 0xa,
    0xa7, 0x0,

    /* U+0031 "1" */
    0x8, 0xf0, 0x4, 0x7b, 0x0, 0x0, 0xb0, 0x0,
    0xb, 0x0, 0x0, 0xb0, 0x0, 0xb, 0x0, 0x3b,
    0xeb, 0x30,

    /* U+0032 "2" */
    0xa, 0xb8, 0x4, 0x70, 0xb1, 0x0, 0xb, 0x0,
    0x3, 0xa0, 0x1, 0xc1, 0x0, 0xb3, 0x0, 0x4e,
    0xbb, 0x20,

    /* U+0033 "3" */
    0x3b, 0xbe, 0x0, 0x7, 0x50, 0x3, 0xd1, 0x0,
    0x15, 0xc0, 0x0, 0xa, 0x5, 0x60, 0xb0, 0xb,
    0xb7, 0x0,

    /* U+0034 "4" */
    0x0, 0x84, 0x1, 0xb0, 0x7, 0x50, 0xc, 0x8,
    0x66, 0xb, 0x6c, 0xbd, 0x0, 0xb,

    /* U+0035 "5" */
    0x3d, 0xba, 0x3, 0x70, 0x0, 0x37, 0x0, 0x2,
    0xcb, 0x70, 0x0, 0xb, 0x14, 0x40, 0xa1, 0xb,
    0xb8, 0x0,

    /* U+0036 "6" */
    0x0, 0xc0, 0x0, 0x49, 0x0, 0xc, 0x20, 0x3,
    0xe9, 0x60, 0x66, 0x9, 0x25, 0x50, 0x82, 0xa,
    0xb9, 0x0,

    /* U+0037 "7" */
    0x5d, 0xbd, 0x54, 0x40, 0xa2, 0x0, 0xb, 0x0,
    0x5, 0x70, 0x0, 0xa2, 0x0, 0xb, 0x0, 0x5,
    0x70, 0x0,

    /* U+0038 "8" */
    0x9, 0xb7, 0x3, 0x80, 0xb0, 0x29, 0xb, 0x0,
    0xbd, 0x80, 0x56, 0xa, 0x25, 0x50, 0x92, 0xa,
    0xb9, 0x0,

    /* U+0039 "9" */
    0xa, 0xb8, 0x4, 0x60, 0xa2, 0x74, 0x7, 0x34,
    0x70, 0xa0, 0x8, 0xa9, 0x0, 0x8, 0x10, 0x2,
    0x90, 0x0,

    /* U+003A ":" */
    0x2e, 0x0, 0x30, 0x0, 0x0, 0x30, 0x2e, 0x0,

    /* U+003B ";" */
    0x2e, 0x0, 0x40, 0x0, 0x0, 0x0, 0x9, 0x1,
    0xb0, 0x47, 0x0,

    /* U+003C "<" */
    0x0, 0x0, 0x0, 0x6, 0xb1, 0x2b, 0x50, 0x2,
    0xb4, 0x0, 0x0, 0x8a, 0x0, 0x0, 0x10,

    /* U+003D "=" */
    0x3b, 0xbb, 0x10, 0x0, 0x0, 0x3b, 0xbb, 0x10,

    /* U+003E ">" */
    0x10, 0x0, 0x2, 0xb4, 0x0, 0x0, 0x8b, 0x0,
    0x5, 0xb1, 0x2b, 0x60, 0x1, 0x0, 0x0,

    /* U+003F "?" */
    0xbc, 0x40, 0xb, 0x0, 0xb4, 0xc3, 0x34, 0x0,
    0x0, 0x77, 0x0,

    /* U+0040 "@" */
    0x9, 0xaa, 0x6, 0x40, 0x65, 0x90, 0x3, 0x7a,
    0x8, 0x97, 0xa0, 0x93, 0x7a, 0x9, 0x36, 0x90,
    0x89, 0x16, 0x50, 0x0, 0xa, 0xa5, 0x0,

    /* U+0041 "A" */
    0x3, 0xf0, 0x0, 0x6a, 0x30, 0x9, 0x46, 0x0,
    0xa0, 0x90, 0xd, 0xac, 0x4, 0x60, 0x91, 0x73,
    0x6, 0x40,

    /* U+0042 "B" */
    0x4d, 0xb8, 0x4, 0x70, 0xb0, 0x47, 0xb, 0x4,
    0xdc, 0x70, 0x47, 0xa, 0x14, 0x70, 0x92, 0x4d,
    0xba, 0x0,

    /* U+0043 "C" */
    0xa, 0xb9, 0x3, 0x80, 0x91, 0x47, 0x0, 0x4,
    0x70, 0x0, 0x47, 0x0, 0x3, 0x80, 0x91, 0xa,
    0xb9, 0x0,

    /* U+0044 "D" */
    0x4d, 0xc6, 0x4, 0x70, 0xb0, 0x47, 0xa, 0x14,
    0x70, 0xa1, 0x47, 0xa, 0x14, 0x70, 0xb0, 0x4d,
    0xb6, 0x0,

    /* U+0045 "E" */
    0x3d, 0xbb, 0x13, 0x80, 0x0, 0x38, 0x0, 0x3,
    0xdb, 0xa0, 0x38, 0x0, 0x3, 0x80, 0x0, 0x3d,
    0xbb, 0x10,

    /* U+0046 "F" */
    0x3d, 0xbb, 0x23, 0x70, 0x0, 0x37, 0x0, 0x3,
    0xdb, 0xb0, 0x37, 0x0, 0x3, 0x70, 0x0, 0x37,
    0x0, 0x0,

    /* U+0047 "G" */
    0xa, 0xb8, 0x3, 0x80, 0x91, 0x46, 0x0, 0x4,
    0x68, 0xc1, 0x46, 0x9, 0x23, 0x80, 0xa0, 0xa,
    0xb8, 0x0,

    /* U+0048 "H" */
    0x47, 0xa, 0x14, 0x70, 0xa1, 0x47, 0xa, 0x14,
    0xdb, 0xe1, 0x47, 0xa, 0x14, 0x70, 0xa1, 0x47,
    0xa, 0x10,

    /* U+0049 "I" */
    0x1b, 0xeb, 0x0, 0xb0, 0x0, 0xb0, 0x0, 0xb0,
    0x0, 0xb0, 0x0, 0xb0, 0x1b, 0xeb,

    /* U+004A "J" */
    0x0, 0xb, 0x0, 0xb, 0x0, 0xb, 0x0, 0xb,
    0x0, 0xb, 0x83, 0xb, 0x2b, 0xc5,

    /* U+004B "K" */
    0x47, 0x9, 0x34, 0x70, 0xb0, 0x47, 0x65, 0x4,
    0xde, 0x10, 0x47, 0x66, 0x4, 0x70, 0xc0, 0x47,
    0x8, 0x40,

    /* U+004C "L" */
    0xb0, 0x0, 0xb0, 0x0, 0xb0, 0x0, 0xb0, 0x0,
    0xb0, 0x0, 0xb0, 0x0, 0xeb, 0xb4,

    /* U+004D "M" */
    0x6b, 0xe, 0x36, 0xb4, 0xb3, 0x68, 0xb8, 0x36,
    0x4b, 0x73, 0x64, 0x7, 0x36, 0x40, 0x73, 0x64,
    0x7, 0x30,

    /* U+004E "N" */
    0x4d, 0x9, 0x14, 0xe2, 0x91, 0x49, 0x69, 0x14,
    0x6a, 0x91, 0x46, 0x9a, 0x14, 0x65, 0xe1, 0x46,
    0xf, 0x10,

    /* U+004F "O" */
    0xa, 0xb8, 0x3, 0x80, 0xb0, 0x46, 0xa, 0x14,
    0x60, 0xa1, 0x46, 0xa, 0x13, 0x80, 0xb0, 0xa,
    0xb8, 0x0,

    /* U+0050 "P" */
    0x4d, 0xbb, 0x4, 0x70, 0x75, 0x47, 0x8, 0x44,
    0xdb, 0x90, 0x47, 0x0, 0x4, 0x70, 0x0, 0x47,
    0x0, 0x0,

    /* U+0051 "Q" */
    0xa, 0xb8, 0x4, 0x70, 0xa1, 0x55, 0x9, 0x25,
    0x50, 0x92, 0x55, 0x9, 0x24, 0x70, 0xa0, 0xa,
    0xc9, 0x0, 0x3, 0x90, 0x0, 0xc, 0x0,

    /* U+0052 "R" */
    0x4d, 0xb9, 0x4, 0x70, 0x92, 0x47, 0xa, 0x24,
    0xdd, 0x90, 0x47, 0x56, 0x4, 0x70, 0xc0, 0x47,
    0x9, 0x20,

    /* U+0053 "S" */
    0x9, 0xb7, 0x3, 0x70, 0x80, 0x2b, 0x10, 0x0,
    0x6d, 0x90, 0x0, 0xb, 0x25, 0x60, 0x92, 0xb,
    0xb9, 0x0,

    /* U+0054 "T" */
    0x6b, 0xeb, 0x40, 0xb, 0x0, 0x0, 0xb0, 0x0,
    0xb, 0x0, 0x0, 0xb0, 0x0, 0xb, 0x0, 0x0,
    0xb0, 0x0,

    /* U+0055 "U" */
    0x47, 0xa, 0x14, 0x70, 0xa1, 0x47, 0xa, 0x14,
    0x70, 0xa1, 0x47, 0xa, 0x13, 0x80, 0xc0, 0xa,
    0xb8, 0x0,

    /* U+0056 "V" */
    0x73, 0x7, 0x44, 0x70, 0xa1, 0x1a, 0xb, 0x0,
    0xb0, 0xa0, 0xa, 0x56, 0x0, 0x6b, 0x30, 0x3,
    0xf0, 0x0,

    /* U+0057 "W" */
    0xa0, 0xd0, 0x9a, 0x2d, 0x27, 0x94, 0xa4, 0x67,
    0x77, 0x74, 0x6b, 0x4a, 0x24, 0xc1, 0xd1, 0x3c,
    0xf, 0x0,

    /* U+0058 "X" */
    0x57, 0x9, 0x20, 0xc1, 0xa0, 0x7, 0xc3, 0x0,
    0x1e, 0x0, 0x8, 0xb5, 0x0, 0xb1, 0xb0, 0x75,
    0x9, 0x40,

    /* U+0059 "Y" */
    0x83, 0x6, 0x51, 0xa0, 0xb0, 0xa, 0x48, 0x0,
    0x4e, 0x10, 0x0, 0xc0, 0x0, 0xb, 0x0, 0x0,
    0xb0, 0x0,

    /* U+005A "Z" */
    0x3b, 0xbe, 0x0, 0x1, 0xa0, 0x0, 0x83, 0x0,
    0x1b, 0x0, 0x8, 0x40, 0x0, 0xb0, 0x0, 0x4d,
    0xbb, 0x10,

    /* U+005B "[" */
    0x4c, 0x65, 0x50, 0x55, 0x5, 0x50, 0x55, 0x5,
    0x50, 0x55, 0x5, 0x50, 0x4c, 0x60,

    /* U+005C "\\" */
    0x23, 0x0, 0x1, 0xa0, 0x0, 0xb, 0x0, 0x0,
    0x65, 0x0, 0x1, 0xa0, 0x0, 0xb, 0x0, 0x0,
    0x65, 0x0, 0x1, 0xa0, 0x0, 0xb, 0x0,

    /* U+005D "]" */
    0x9c, 0x20, 0x92, 0x9, 0x20, 0x92, 0x9, 0x20,
    0x92, 0x9, 0x20, 0x92, 0x9c, 0x20,

    /* U+005E "^" */
    0x0, 0x60, 0x0, 0x5b, 0x20, 0x9, 0x18, 0x2,
    0x70, 0x90,

    /* U+005F "_" */
    0x5a, 0xaa, 0x30,

    /* U+0060 "`" */
    0x41, 0x2a,

    /* U+0061 "a" */
    0xa, 0xb9, 0x1, 0x30, 0xb0, 0x1a, 0xad, 0x16,
    0x60, 0xb1, 0x2c, 0xac, 0x10,

    /* U+0062 "b" */
    0x47, 0x0, 0x4, 0x70, 0x0, 0x4a, 0xb9, 0x4,
    0x80, 0xa1, 0x47, 0x9, 0x14, 0x80, 0xa1, 0x4b,
    0xb9, 0x0,

    /* U+0063 "c" */
    0xa, 0xb9, 0x3, 0x80, 0x71, 0x46, 0x0, 0x3,
    0x80, 0x71, 0xa, 0xb9, 0x0,

    /* U+0064 "d" */
    0x0, 0xa, 0x10, 0x0, 0xa1, 0xb, 0xac, 0x14,
    0x70, 0xb1, 0x46, 0xa, 0x14, 0x70, 0xb1, 0xb,
    0xac, 0x10,

    /* U+0065 "e" */
    0xa, 0xa7, 0x4, 0x70, 0xa0, 0x5c, 0xab, 0x14,
    0x70, 0x30, 0xa, 0xb9, 0x0,

    /* U+0066 "f" */
    0x0, 0xcb, 0x20, 0x47, 0x0, 0x4, 0x70, 0x6,
    0xcd, 0xb2, 0x4, 0x70, 0x0, 0x47, 0x0, 0x4,
    0x70, 0x0,

    /* U+0067 "g" */
    0xb, 0xac, 0x13, 0x80, 0xb1, 0x46, 0xa, 0x13,
    0x90, 0xc1, 0x9, 0x9b, 0x10, 0x0, 0xb0, 0x8,
    0xc8, 0x0,

    /* U+0068 "h" */
    0x47, 0x0, 0x4, 0x70, 0x0, 0x4a, 0xb9, 0x4,
    0x80, 0xb0, 0x47, 0xa, 0x14, 0x70, 0xa1, 0x47,
    0xa, 0x10,

    /* U+0069 "i" */
    0x0, 0xd0, 0x0, 0x1, 0x0, 0x1b, 0xe0, 0x0,
    0xb, 0x0, 0x0, 0xb0, 0x0, 0xb, 0x0, 0x3b,
    0xeb, 0x50,

    /* U+006A "j" */
    0x0, 0x67, 0x0, 0x0, 0x3b, 0xc7, 0x0, 0x37,
    0x0, 0x37, 0x0, 0x37, 0x0, 0x37, 0x0, 0x56,
    0x3b, 0xb0,

    /* U+006B "k" */
    0x38, 0x0, 0x3, 0x80, 0x0, 0x38, 0xb, 0x23,
    0x84, 0x80, 0x3d, 0xd1, 0x3, 0x84, 0x90, 0x38,
    0xa, 0x20,

    /* U+006C "l" */
    0x8c, 0x70, 0x0, 0x37, 0x0, 0x3, 0x70, 0x0,
    0x37, 0x0, 0x3, 0x70, 0x0, 0x38, 0x0, 0x0,
    0xcb, 0x40,

    /* U+006D "m" */
    0x89, 0xab, 0x18, 0x29, 0x54, 0x82, 0x95, 0x58,
    0x29, 0x55, 0x82, 0x95, 0x50,

    /* U+006E "n" */
    0x4a, 0x99, 0x4, 0x80, 0xb0, 0x47, 0xa, 0x14,
    0x70, 0xa1, 0x47, 0xa, 0x10,

    /* U+006F "o" */
    0xa, 0xb8, 0x4, 0x70, 0xa0, 0x56, 0x9, 0x24,
    0x70, 0xa0, 0xa, 0xb8, 0x0,

    /* U+0070 "p" */
    0x4b, 0x99, 0x4, 0x80, 0xa1, 0x47, 0x9, 0x14,
    0x90, 0xa1, 0x4b, 0xb9, 0x4, 0x70, 0x0, 0x47,
    0x0, 0x0,

    /* U+0071 "q" */
    0xb, 0xac, 0x14, 0x70, 0xb1, 0x46, 0xa, 0x14,
    0x70, 0xb1, 0xb, 0xac, 0x10, 0x0, 0xa1, 0x0,
    0xa, 0x10,

    /* U+0072 "r" */
    0x1b, 0x9b, 0x1, 0xa0, 0x83, 0x19, 0x0, 0x1,
    0x90, 0x0, 0x19, 0x0, 0x0,

    /* U+0073 "s" */
    0xb, 0xba, 0x3, 0x80, 0x30, 0x9, 0xba, 0x1,
    0x30, 0xa1, 0x1c, 0xba, 0x0,

    /* U+0074 "t" */
    0x2, 0x20, 0x0, 0x55, 0x0, 0x7d, 0xdb, 0x10,
    0x55, 0x0, 0x5, 0x50, 0x0, 0x55, 0x0, 0x2,
    0xcb, 0x10,

    /* U+0075 "u" */
    0x47, 0xa, 0x14, 0x70, 0xa1, 0x47, 0xa, 0x13,
    0x80, 0xb0, 0xa, 0xb8, 0x0,

    /* U+0076 "v" */
    0x65, 0x8, 0x31, 0xa0, 0xb0, 0xb, 0x19, 0x0,
    0x89, 0x50, 0x3, 0xf0, 0x0,

    /* U+0077 "w" */
    0x90, 0xd1, 0x78, 0x4c, 0x45, 0x68, 0x78, 0x23,
    0xc3, 0xc0, 0x1d, 0xd, 0x0,

    /* U+0078 "x" */
    0x39, 0xb, 0x10, 0x98, 0x60, 0x3, 0xf0, 0x0,
    0xa7, 0x70, 0x48, 0xb, 0x20,

    /* U+0079 "y" */
    0x65, 0x8, 0x31, 0xa0, 0xb0, 0xb, 0x29, 0x0,
    0x6b, 0x40, 0x1, 0xe0, 0x0, 0x1a, 0x0, 0x6,
    0x50, 0x0,

    /* U+007A "z" */
    0x2b, 0xbf, 0x0, 0x5, 0x70, 0x2, 0xb0, 0x0,
    0xb1, 0x0, 0x4e, 0xbb, 0x0,

    /* U+007B "{" */
    0x0, 0x5b, 0x0, 0xb, 0x0, 0x0, 0xb0, 0x0,
    0xb, 0x0, 0x4b, 0x80, 0x0, 0xb, 0x0, 0x0,
    0xb0, 0x0, 0xb, 0x0, 0x0, 0x5b, 0x0,

    /* U+007C "|" */
    0x5b, 0xbb, 0xbb, 0xbb, 0xb0,

    /* U+007D "}" */
    0x2b, 0x20, 0x0, 0x28, 0x0, 0x3, 0x70, 0x0,
    0x46, 0x0, 0x0, 0xcb, 0x20, 0x46, 0x0, 0x3,
    0x70, 0x0, 0x28, 0x0, 0x2b, 0x20, 0x0,

    /* U+007E "~" */
    0x2b, 0x55, 0x35, 0x38, 0xa0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 77, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 0, .adv_w = 77, .box_w = 2, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 7, .adv_w = 77, .box_w = 3, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 12, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 30, .adv_w = 77, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 53, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 71, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 89, .adv_w = 77, .box_w = 2, .box_h = 3, .ofs_x = 1, .ofs_y = 4},
    {.bitmap_index = 92, .adv_w = 77, .box_w = 3, .box_h = 10, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 107, .adv_w = 77, .box_w = 4, .box_h = 10, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 127, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 140, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 153, .adv_w = 77, .box_w = 2, .box_h = 3, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 156, .adv_w = 77, .box_w = 3, .box_h = 1, .ofs_x = 1, .ofs_y = 2},
    {.bitmap_index = 158, .adv_w = 77, .box_w = 3, .box_h = 2, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 161, .adv_w = 77, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 184, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 202, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 220, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 238, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 256, .adv_w = 77, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 270, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 288, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 306, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 324, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 342, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 360, .adv_w = 77, .box_w = 3, .box_h = 5, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 368, .adv_w = 77, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = -2},
    {.bitmap_index = 379, .adv_w = 77, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 394, .adv_w = 77, .box_w = 5, .box_h = 3, .ofs_x = 0, .ofs_y = 1},
    {.bitmap_index = 402, .adv_w = 77, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 417, .adv_w = 77, .box_w = 3, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 428, .adv_w = 77, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 451, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 469, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 487, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 505, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 523, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 541, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 559, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 577, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 595, .adv_w = 77, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 609, .adv_w = 77, .box_w = 4, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 623, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 641, .adv_w = 77, .box_w = 4, .box_h = 7, .ofs_x = 1, .ofs_y = 0},
    {.bitmap_index = 655, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 673, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 691, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 709, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 727, .adv_w = 77, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 750, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 768, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 786, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 804, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 822, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 840, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 858, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 876, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 894, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 912, .adv_w = 77, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 926, .adv_w = 77, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 949, .adv_w = 77, .box_w = 3, .box_h = 9, .ofs_x = 1, .ofs_y = -1},
    {.bitmap_index = 963, .adv_w = 77, .box_w = 5, .box_h = 4, .ofs_x = 0, .ofs_y = 3},
    {.bitmap_index = 973, .adv_w = 77, .box_w = 5, .box_h = 1, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 976, .adv_w = 77, .box_w = 2, .box_h = 2, .ofs_x = 1, .ofs_y = 6},
    {.bitmap_index = 978, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 991, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1009, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1022, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1040, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1053, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1071, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1089, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1107, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1125, .adv_w = 77, .box_w = 4, .box_h = 9, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1143, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1161, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1179, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1192, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1205, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1218, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1236, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1254, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1267, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1280, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1298, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1311, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1324, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1337, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1350, .adv_w = 77, .box_w = 5, .box_h = 7, .ofs_x = 0, .ofs_y = -2},
    {.bitmap_index = 1368, .adv_w = 77, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 0},
    {.bitmap_index = 1381, .adv_w = 77, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1404, .adv_w = 77, .box_w = 1, .box_h = 9, .ofs_x = 2, .ofs_y = -1},
    {.bitmap_index = 1409, .adv_w = 77, .box_w = 5, .box_h = 9, .ofs_x = 0, .ofs_y = -1},
    {.bitmap_index = 1432, .adv_w = 77, .box_w = 5, .box_h = 2, .ofs_x = 0, .ofs_y = 2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
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
const lv_font_t JetBrainsMono8 = {
#else
lv_font_t JetBrainsMono8 = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 10,          /*The maximum line height required by the font*/
    .base_line = 2,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
    .subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
    .underline_position = -1,
    .underline_thickness = 0,
#endif
    .dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
    .fallback = NULL,
    .user_data = NULL
};



#endif /*#if JETBRAINSMONO8*/

