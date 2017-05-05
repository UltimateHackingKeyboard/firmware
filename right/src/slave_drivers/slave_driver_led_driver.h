#ifndef __SLAVE_DRIVER_LED_DRIVER_H__
#define __SLAVE_DRIVER_LED_DRIVER_H__

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

    extern void InitLedSlaveDriver();
    extern void UhkSlaveLedDriverHandler(uint8_t ledDriverId);
    extern void SetLeds(uint8_t ledBrightness);

#endif
