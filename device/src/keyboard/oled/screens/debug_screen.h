#ifndef __DEBUG_SCREEN_H__
#define __DEBUG_SCREEN_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "keyboard/oled/widgets/widget.h"

// Macros:

// Typedefs:

    #define DEBUG_SCREEN_NOTIFICATION_TIMEOUT 10*1000

// Variables:

    extern widget_t* DebugScreen;

// Functions:

    void DebugScreen_Init();

#endif
