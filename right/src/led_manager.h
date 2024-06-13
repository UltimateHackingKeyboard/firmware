#ifndef __LED_MANAGER_H__
#define __LED_MANAGER_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>
    #include "attributes.h"

// Macros:
// Variables:

    extern uint8_t DisplayBrightness;
    extern uint8_t KeyBacklightBrightness;

    extern bool KeyBacklightSleepModeActive;
    extern bool DisplaySleepModeActive;
    extern bool LedManager_FullUpdateNeeded;

// Functions:

    void LedManager_FullUpdate();
    void LedManager_RecalculateLedBrightness();


#endif
