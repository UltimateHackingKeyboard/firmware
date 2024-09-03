#ifndef __STUBS_H__
#define __STUBS_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include <stddef.h>
    #include "attributes.h"
    #include "key_action.h"

// Macros:

#define ATTRS __attribute__((weak)) __attribute__((unused))

// Variables:

// Functions:

    ATTRS bool SegmentDisplay_NeedsUpdate = false;
    ATTRS bool RunningOnBattery = false;
    ATTRS void Oled_UpdateBrightness() {};
    ATTRS void Oled_ShiftScreen() {};
    ATTRS void ScreenManager_SwitchScreenEvent() {};
    ATTRS void Charger_UpdateBatteryState() {};
    ATTRS const rgb_t* PairingScreen_ActionColor(key_action_t* action) { return NULL; };
    ATTRS void Uart_Reenable() {};
    ATTRS void Uart_Enable() {};

#if DEVICE_HAS_OLED
#define WIDGET_REFRESH(W) Widget_Refresh(W)
#else
#define WIDGET_REFRESH(W)
#endif

#endif
