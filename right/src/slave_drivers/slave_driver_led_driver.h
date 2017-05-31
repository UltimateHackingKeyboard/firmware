#ifndef __SLAVE_DRIVER_LED_DRIVER_H__
#define __SLAVE_DRIVER_LED_DRIVER_H__

// Includes:

    #include "fsl_common.h"

// Typedefs:

    typedef enum {
        LedDriverPhase_SetFunctionFrame,
        LedDriverPhase_SetShutdownModeNormal,
        LedDriverPhase_SetFrame1,
        LedDriverPhase_InitLedControlRegisters,
        LedDriverPhase_Initialized,
    } led_driver_phase_t;

    typedef enum {

    } led_driver_state_t;

// Functions:

    extern void LedSlaveDriver_Init();
    extern void LedSlaveDriver_Update(uint8_t ledDriverId);
    extern void SetLeds(uint8_t ledBrightness);

#endif
