#include "text_widget.h"
#include "keyboard/oled/widgets/widgets.h"
#include "keyboard/oled/oled.h"
#include <stdlib.h>
#include "str_utils.h"

void TextWidget_Draw(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        if (self->textData.textProvider != NULL) {
            self->textData.text = self->textData.textProvider();
        }
        if (self->textData.text.start != NULL) {
            Framebuffer_DrawText(self, buffer, AlignmentType_Center, AlignmentType_Center, self->textData.font, self->textData.text.start, self->textData.text.end);
        }
    }
}

void TextWidget_SetText(widget_t* self, char* text)
{
    self->dirty = true;
    self->textData.text.start = text;
    Oled_RequestRedraw();
}

widget_t TextWidget_Build(const lv_font_t* font, char* text)
{
    return (widget_t){
        .type = WidgetType_Text,
        .layOut = &CustomWidget_LayOut,
        .draw = &TextWidget_Draw,
        .dirty = true,
        .textData = {
            .font = font,
            .text = { .start = text, .end = NULL },
            .textProvider = NULL,
        }
    };
}

widget_t TextWidget_BuildRefreshable(const lv_font_t* font, string_segment_t (*textProvider)()) {
    return (widget_t){
        .type = WidgetType_Text,
        .layOut = &CustomWidget_LayOut,
        .draw = &TextWidget_Draw,
        .dirty = true,
        .textData = {
            .font = font,
            .text = { .start = NULL, .end = NULL },
            .textProvider = textProvider,
        }
    };
}


