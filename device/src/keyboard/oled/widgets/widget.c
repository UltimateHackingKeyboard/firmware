#include "widget.h"
#include "../oled.h"
#include "device.h"
#include <stdlib.h>

void Widget_Refresh(widget_t* widget)
{
#if DEVICE_HAS_OLED
    if (widget != NULL) {
        widget->dirty = true;
    }
    Oled_RequestRedraw();
#endif
}
