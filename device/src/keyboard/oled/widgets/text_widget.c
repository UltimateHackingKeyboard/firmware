#include "text_widget.h"
#include "keyboard/oled/widgets/widgets.h"
#include <stdlib.h>

void TextWidget_Draw(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        Framebuffer_Clear(self, buffer);
        Framebuffer_DrawTextAnchored(self, buffer, AnchorType_Center, AnchorType_Center, self->textData.font, self->textData.text, NULL);
    }
}

void TextWidget_SetText(widget_t* self, char* text)
{
    self->dirty = true;
    self->textData.text = text;
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
            .text = text,
        }
    };
}
