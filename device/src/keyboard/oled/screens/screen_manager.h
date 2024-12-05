#ifndef __SCREEN_MANAGER_H__
#define __SCREEN_MANAGER_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "../widgets/widget.h"

// Macros:

#define SCREEN_NOTIFICATION_TIMEOUT 3000

// Typedefs:

    typedef enum {
        ScreenId_Main,
        ScreenId_Debug,
        ScreenId_Pairing,
        ScreenId_PairingFailed,
        ScreenId_PairingSucceeded,
        ScreenId_Canvas,
    } screen_id_t;

// Variables:

    extern bool ScreenManager_AwaitsInput;
    extern screen_id_t ActiveScreen;

// Functions:

    void ScreenManager_ActivateScreen(screen_id_t screen);
    void ScreenManager_SwitchScreenEvent();
    void ScreenManager_Init();

#endif
