#ifndef __TEXT_WIDGET_H__
#define __TEXT_WIDGET_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "widget.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

    void TextWidget_SetText(widget_t* self, char* text);
    widget_t TextWidget_Build(const lv_font_t* font, char* text);

#endif
