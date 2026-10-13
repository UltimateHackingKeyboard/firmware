#ifndef __NOTIFICATION_SCREEN_H__
#define __NOTIFICATION_SCREEN_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "../widgets/widget.h"

// Macros:

// Typedefs:

    // Notify functions return ids from the Valid range. They are held in uint8_t.
    typedef enum {
        NotificationId_Invalid = 0,
        NotificationId_FirstValid = 1,
        NotificationId_LastValid = 254,
        NotificationId_Any = 255,
    } notification_id_t;

// Variables:

    extern widget_t* NotificationScreen;

    // The display has to stay lit for the notification even if it would be off otherwise.
    extern bool NotificationScreen_ForceAwake;

// Functions:

    // Hide hides the notification only if it still is the one identified by seqId. See notification_id_t.
    void NotificationScreen_Init(void);
    uint8_t NotificationScreen_Notify(const char* message);
    uint8_t NotificationScreen_NotifyFor(const char* message, uint16_t duration, bool forceAwake);
    void NotificationScreen_Hide(uint8_t seqId);
    void NotificationScreen_EndForceAwake(void);

#endif
