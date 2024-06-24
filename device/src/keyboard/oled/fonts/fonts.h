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
 *
 *  - f057 - circle-xmark
 *  - f8dd - signal-stream
 *  - f1e6 - plug
 *
 *  Commands to create icon fonts:
 *  `
 *  fontforge -lang=ff -c 'Open($1); Generate($2);' Font\ Awesome\ 6\ Pro-Regular-400.otf font_awesome_6_reguler.ttf
 *  npx lv_font_conv --lv-font-name FontAwesome12 --format lvgl --bpp 4 -o font_awesome_12.c --size 12 --font /opt/fontawesome/otfs/font_awesome_6_reguler.ttf --range 0xf057,0xf8dd,0xf1e6 --no-compress
 *  */

    extern const lv_font_t JetBrainsMono8;
    extern const lv_font_t JetBrainsMono10;
    extern const lv_font_t JetBrainsMono12;
    extern const lv_font_t JetBrainsMono16;
    extern const lv_font_t JetBrainsMono24;
    extern const lv_font_t JetBrainsMono32;
    extern const lv_font_t CustomMono8;
    extern const lv_font_t FontAwesome12;

#endif
