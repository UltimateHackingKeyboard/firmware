#ifndef __POWER_MODE_H__
#define __POWER_MODE_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>

// Macros:

    #define POWER_MODE_UPDATE_DELAY 500
    #define POWER_MODE_RESTART_DELAY 2500

// Typedefs:

    typedef struct {
        const char* name;
        uint16_t i2cInterval;
        uint16_t keyScanInterval;
    } power_mode_config_t;

    typedef enum {
        PowerMode_Awake,
        PowerMode_Powersaving,
        PowerMode_LastAwake = PowerMode_Powersaving,
        PowerMode_LightSleep,
        PowerMode_Uhk60Sleep = PowerMode_LightSleep,
        PowerMode_Lock,
        PowerMode_SfjlSleep,
        PowerMode_AutoShutDown,
        PowerMode_ManualShutDown,
        PowerMode_Count,
    } power_mode_t;

// Variables:

    extern power_mode_t CurrentPowerMode;
    extern power_mode_config_t PowerModeConfig[PowerMode_Count];

// Functions:

    void PowerMode_SetUsbAwake(bool awake);
    void PowerMode_Update();
    void PowerMode_ActivateMode(power_mode_t mode, bool toggle);
    void PowerMode_WakeHost();

    void PowerMode_RestartedTo(power_mode_t mode);
    void PowerMode_Restart();

#endif
