#ifndef __CONSOLE_WIDGET_H__
#define __CONSOLE_WIDGET_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "widget.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

    widget_t ConsoleWidget_Build();

    void Oled_LogConstant(const char* text);
    void Oled_Log(const char *fmt, ...);

#endif
