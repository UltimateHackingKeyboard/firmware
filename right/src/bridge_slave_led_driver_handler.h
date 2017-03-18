#ifndef __BRIDGE_SLAVE_LED_DRIVER_HANDLER_H__
#define __BRIDGE_SLAVE_LED_DRIVER_HANDLER_H__

// Includes:

    #include "fsl_common.h"

// Typedefs:

typedef enum {
    LedDriverState_SetFunctionFrame,
    LedDriverState_SetShutdownModeNormal,
    LedDriverState_SetFrame1,
    LedDriverState_InitLedControlRegisters,
    LedDriverState_Initialized,
} LedDriverState;

// Functions:

    extern bool BridgeSlaveLedDriverHandler(uint8_t ledDriverId);
    extern void SetLeds(uint8_t ledBrightness);

#endif
