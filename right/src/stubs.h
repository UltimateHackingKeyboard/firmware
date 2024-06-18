#ifndef __STUBS_H__
#define __STUBS_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "attributes.h"
    #include "key_action.h"

// Macros:
// Variables:
// Functions:


#if !defined(__ZEPHYR__) || DEVICE_IS_UHK_DONGLE
     __attribute__((weak)) ATTR_UNUSED bool RunningOnBattery = false;

#endif

#ifndef DEVICE_HAS_OLED
    static ATTR_UNUSED void Oled_UpdateBrightness() {};
#endif

    ATTR_UNUSED __attribute__((weak)) const rgb_t* PairingScreen_ActionColor(key_action_t* action);

#endif
