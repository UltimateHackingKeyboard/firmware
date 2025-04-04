#ifndef __NOTIFICATION_SCREEN_H__
#define __NOTIFICATION_SCREEN_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "../widgets/widget.h"

// Macros:

// Typedefs:

// Variables:

    extern widget_t* NotificationScreen;

// Functions:

    void NotificationScreen_Init(void);
    void NotificationScreen_Notify(const char* message);

#endif
