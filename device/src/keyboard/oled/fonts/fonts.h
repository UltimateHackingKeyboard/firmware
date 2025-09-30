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
 git clone git@github.com:lvgl/lv_font_conv.git
 cd lv_font_conv
 git checkout 968adde4d1d6e01af18774ce83e51da119cf5d91
 npm ci
 npx lv_font_conv --lv-font-name JetBrainsMono10 --format lvgl --bpp 4 -o jet_brains_mono_10.c --size 10 --font ../fonts/ttf/JetBrainsMono-Regular.ttf --range 0x20-0x7F --no-compress
 *  ```
 *
 *  - f057 - circle-xmark
 *  - f8dd - signal-stream
 *  - f1e6 - plug
 *  - f071 - triangle-exclamation
 *  - f06b - gift
 *  - e0b1 - battery-low
 *  - e0ee - circle-1
 *  - e0ef - circle-2
 *  - f03d - video
 *  - f8ab - casette-tape
 *
 *  customs:
 *  - e000 - circle-xmark-large (custom)
 *  - e001,e002 - caps lock, numlock
 *  - e003 - connected plug
 *
 *  Script to create icon fonts, to be run from the root of a decompressed fontawesome folder:
```
#!/bin/bash

echo "Please download the kit from https://fontawesome.com/kits/4711ad2078/download and press enter"
read a

unzip kit-4711ad2078-desktop.zip

mkdir ttfs

fontforge -lang=ff -c 'Open($1); Generate($2);' otfs/Font\ Awesome\ 6\ Pro-Regular-400.otf ttfs/font_awesome_6_regular.ttf
fontforge -lang=ff -c 'Open($1); Generate($2);' otfs/Font\ Awesome\ 6\ Pro-Solid-900.otf ttfs/font_awesome_6_solid.ttf
fontforge -lang=ff -c 'Open($1); Generate($2);' otfs/Font\ Awesome\ Kit\ 4711ad2078-Regular-400.otf ttfs/font_awesome_custom.ttf

npx lv_font_conv --lv-font-name CustomIcons --format lvgl --bpp 4 -o icons_custom.c --size 12 --font ttfs/font_awesome_custom.ttf --range 0xe000-0xe009 --no-compress
npx lv_font_conv --lv-font-name RegularIcons --format lvgl --bpp 4 -o icons_regular.c --size 12 --font ttfs/font_awesome_6_regular.ttf --range 0xf057,0xf8dd,0xf1e6,0xf071,0xf06b,0xf293,0xe0b1 --no-compress
npx lv_font_conv --lv-font-name SolidIcons --format lvgl --bpp 4 -o icons_solid.c --size 12 --font ttfs/font_awesome_6_solid.ttf --range 0xe423,0xe422,0xf0e7,0xe0ee,0xe0ef,0xf03d,0xf8ab --no-compress
```

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
