#include "widget.h"
#include "../oled.h"
#include <stdlib.h>

void Widget_RequestRedraw(widget_t* widget)
{
    if (widget != NULL) {
        widget->dirty = true;
    }
    OledNeedsRedraw = true;
}
