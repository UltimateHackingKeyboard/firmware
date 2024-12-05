#ifndef __WIDGET_STOR_H__
#define __WIDGET_STOR_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "widget.h"

// Macros:

// Typedefs:

// Variables:

    extern widget_t KeymapWidget;
    extern widget_t LayerWidget;
    extern widget_t KeymapLayerWidget;
    extern widget_t StatusWidget;
    extern widget_t CanvasWidget;
    extern widget_t ConsoleWidget;
    extern widget_t TargetWidget;

// Functions:

    void WidgetStore_Init();

#endif


