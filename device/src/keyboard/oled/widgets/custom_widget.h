#ifndef __CUSTOM_WIDGET_H__
#define __CUSTOM_WIDGET_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "widget.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

    void CustomWidget_LayOut(widget_t* self, uint8_t x, uint8_t y, uint8_t w, uint8_t h);
    widget_t CustomWidget_Build(void (*draw)(widget_t* self, framebuffer_t* buffer));

#endif
