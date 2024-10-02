#ifndef __UHK_POWER_MODE_H__
#define __UHK_POWER_MODE_H__

// Includes:

    #include <stdbool.h>

// Macros:

    #define POWER_MODE_UPDATE_DELAY 500

// Typedefs:

typedef enum {
    PowerMode_Awake,
    PowerMode_Sleep,
    PowerMode_Uhk60Sleep = PowerMode_Sleep,
    PowerMode_Count,
} power_mode_t;
// Variables:

    extern power_mode_t CurrentPowerMode;

// Functions:

    void PowerMode_SetUsbAwake(bool awake);
    void PowerMode_Update();
    void PowerMode_ActivateMode(power_mode_t mode, bool toggle);
    void PowerMode_WakeHost();

#endif
