#ifndef __POWER_MODE_H__
#define __POWER_MODE_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>

// Macros:

    #define POWER_MODE_UPDATE_DELAY 500

    // Grace period between entering deep sleep and actually cutting the radio and the
    // inter-half link, so that the mode change still reaches the other half.
    #define POWER_MODE_DEEP_SLEEP_DELAY 2500

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
        PowerMode_DeepSleep,
        PowerMode_Count,
    } power_mode_t;

// Variables:

    extern volatile power_mode_t CurrentPowerMode;
    extern power_mode_config_t PowerModeConfig[PowerMode_Count];

// Functions:

    void PowerMode_Update();
    void PowerMode_ActivateMode(power_mode_t mode, bool toggle, bool force, const char* reason);


    void PowerMode_PutBackToSleepMaybe(void);

#ifdef __ZEPHYR__
    // Bluetooth and the inter-half uart bridge; both go down in deep sleep.
    void PowerMode_SetLinksEnabled(bool enabled);
    void PowerMode_EnterDeepSleep(void);
#endif

#endif
