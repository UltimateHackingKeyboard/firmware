#ifndef __TEXT_WIDGET_H__
#define __TEXT_WIDGET_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "widget.h"

// Macros:

// Typedefs:

typedef enum {
    TextWidgetId_Layer,
    TextWidgetId_Status,
    TextWidgetId_Keymap,
    TextWidgetId_Count,
} text_widget_id_t;

// Variables:

// Functions:

    void TextWidget_SetText(widget_t* self, char* text);
    widget_t TextWidget_Build(const lv_font_t* font, char* text);
    widget_t TextWidget_BuildRefreshable(const lv_font_t* font, string_segment_t (*textProvider)());

    void InitTextWidgets();

#endif
