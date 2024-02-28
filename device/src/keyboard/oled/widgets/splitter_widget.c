#include "splitter_widget.h"
#include "widget.h"
#include "keyboard/logger.h"

void SplitterWidget_LayOut(widget_t* self, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    self->x = x;
    self->y = y;
    self->w = w;
    self->h = h;
    uint8_t splitWidth = self->splitterData.splitLine ? 1 : 0;
    if (self->type == WidgetType_HSplitter) {
        self->splitterData.child1->layOut(self->splitterData.child1, x, y, w, self->splitterData.splitAt);
        self->splitterData.child2->layOut(self->splitterData.child2, x, y+self->splitterData.splitAt+splitWidth, w, h-splitWidth-self->splitterData.splitAt);
    } else {
        self->splitterData.child1->layOut(self->splitterData.child1, x, y, self->splitterData.splitAt, h);
        self->splitterData.child2->layOut(self->splitterData.child2, x+self->splitterData.splitAt+splitWidth, y, w-splitWidth-self->splitterData.splitAt, h);
    }

}

void SplitterWidget_Draw(widget_t* self, framebuffer_t* buffer)
{
    if (self->dirty) {
        self->dirty = false;
        if (self->type == WidgetType_HSplitter) {
            Framebuffer_DrawHLine(self, buffer, 0, self->w, self->splitterData.splitAt);
        } else {
            Framebuffer_DrawVLine(self, buffer, self->splitterData.splitAt, 0, self->h);
        }
    }

    self->splitterData.child1->draw(self->splitterData.child1, buffer);
    self->splitterData.child2->draw(self->splitterData.child2, buffer);
}

widget_t SplitterWidget_BuildVertical(widget_t* child1, widget_t* child2, uint8_t splitAt, bool splitLine)
{
    return (widget_t){
        .type = WidgetType_VSplitter,
        .splitterData.child1 = child1,
        .splitterData.child2 = child2,
        .splitterData.splitAt = splitAt,
        .splitterData.splitLine = splitLine,
        .layOut = &SplitterWidget_LayOut,
        .draw = &SplitterWidget_Draw,
        .dirty = true,
    };
}

widget_t SplitterWidget_BuildHorizontal(widget_t* child1, widget_t* child2, uint8_t splitAt, bool splitLine)
{
    return (widget_t){
        .type = WidgetType_HSplitter,
        .splitterData.child1 = child1,
        .splitterData.child2 = child2,
        .splitterData.splitAt = splitAt,
        .splitterData.splitLine = splitLine,
        .layOut = &SplitterWidget_LayOut,
        .draw = &SplitterWidget_Draw,
        .dirty = true,
    };
}
