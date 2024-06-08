#ifndef __STUBS_H__
#define __STUBS_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "attributes.h"

// Macros:
// Variables:
// Functions:


#if !defined(__ZEPHYR__) || DEVICE_IS_UHK_DONGLE
    static ATTR_UNUSED bool RunningOnBattery = false;

#endif

#ifndef DEVICE_HAS_OLED
    static ATTR_UNUSED void Oled_UpdateBrightness() {};
#endif


#endif
