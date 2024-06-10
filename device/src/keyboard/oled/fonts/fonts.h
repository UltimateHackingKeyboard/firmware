#ifndef __FONTS_H__
#define __FONTS_H__

// Includes:

    #include "lvgl/lvgl.h"

// Variables:

/* Instructions to add new font:
 *
 * We use the lvgl font converter from https://lvgl.io/tools/fontconverter. Unfortunately, from some version the font converter stopped generating fonts compatible with our rendering code, so we need to use an older version.
 *
 * ```
 *  git clone git@github.com:lvgl/lv_font_conv.git
 *  cd lv_font_conv
 *  git checkout 968adde4d1d6e01af18774ce83e51da119cf5d91
 *  npm ci
 *  npx lv_font_conv --lv-font-name JetBrainsMono10 --format lvgl --bpp 4 -o jet_brains_mono_10.c --size 10 --font ~/Downloads/fonts/ttf/JetBrainsMono-Regular.ttf --range 0x20-0x7F --no-compress
 *  ```
 *  */

    extern const lv_font_t JetBrainsMono8;
    extern const lv_font_t JetBrainsMono10;
    extern const lv_font_t JetBrainsMono12;
    extern const lv_font_t JetBrainsMono16;
    extern const lv_font_t JetBrainsMono24;
    extern const lv_font_t JetBrainsMono32;
    extern const lv_font_t CustomMono8;

#endif
