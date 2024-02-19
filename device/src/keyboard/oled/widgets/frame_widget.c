#include "frame_widget.h"
#include "widget.h"
#include "keyboard/logger.h"

void FrameWidget_LayOut(widget_t* self, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    self->x = x;
    self->y = y;
    self->w = w;
    self->h = h;
    self->simpleContentData.content->layOut(self->simpleContentData.content, x+1, y+1, w-2, h-2);
}

void FrameWidget_Draw(widget_t* self, framebuffer_t* buffer)
{
    Framebuffer_DrawHLine(self, buffer, 0, self->w, 0);
    Framebuffer_DrawHLine(self, buffer, 0, self->w, self->h-1);
    Framebuffer_DrawVLine(self, buffer, 0, 0, self->h);
    Framebuffer_DrawVLine(self, buffer, self->w-1, 0, self->h-1);
    self->simpleContentData.content->draw(self->simpleContentData.content, buffer);
}

widget_t FrameWidget_Build(widget_t* content)
{
    return (widget_t){
        .type = WidgetType_Frame,
        .simpleContentData.content = content,
        .layOut = &FrameWidget_LayOut,
        .draw = &FrameWidget_Draw,
    };
}
