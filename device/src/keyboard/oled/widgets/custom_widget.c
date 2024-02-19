#include "custom_widget.h"
#include "widget.h"
#include <stdlib.h>

void CustomWidget_LayOut(widget_t* self, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    self->x = x;
    self->y = y;
    self->w = w;
    self->h = h;
}

void CustomWidget_Draw(widget_t* self, framebuffer_t* buffer)
{
}

widget_t CustomWidget_Build(void (*draw)(widget_t* self, framebuffer_t* buffer))
{
    return (widget_t){
        .type = WidgetType_Custom,
        .layOut = &CustomWidget_LayOut,
        .draw = draw == NULL ? &CustomWidget_Draw : draw,
    };
}
