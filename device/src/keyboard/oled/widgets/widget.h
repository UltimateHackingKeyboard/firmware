#ifndef __WIDGET_H__
#define __WIDGET_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "../framebuffer.h"
    #include "keyboard/oled/fonts/fonts.h"
    #include "str_utils.h"

// Macros:

// Typedefs:

    typedef enum {
        WidgetType_VSplitter,
        WidgetType_HSplitter,
        WidgetType_Frame,
        WidgetType_Console,
        WidgetType_Text,
        WidgetType_Custom,
    } widget_type_t;

    typedef struct widget_t widget_t;

    struct widget_t {
        uint8_t x;
        uint8_t y;
        uint8_t w;
        uint8_t h;

        void (*layOut)(widget_t* self, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
        void (*draw)(widget_t* self, framebuffer_t* buffer);

        widget_type_t type;
        bool dirty;

        union {
            struct {
                widget_t* child1;
                widget_t* child2;
                uint8_t splitAt;
                bool splitLine;
            } splitterData;
            struct {
                widget_t* content;
            } simpleContentData;
            struct {
                string_segment_t text;
                const lv_font_t* font;
                string_segment_t (*textProvider)();
            } textData;
        };
    };

// Variables:

// Functions:

    void Widget_Refresh(widget_t* widget);

#endif
