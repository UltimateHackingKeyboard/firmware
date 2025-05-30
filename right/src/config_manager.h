#ifndef __CONFIG_MANAGER_H__
#define __CONFIG_MANAGER_H__

// Includes:

    #include "key_action.h"
    #include "module.h"
    #include "secondary_role_driver.h"
    #include "mouse_keys.h"
    #include "macros/core.h"
    #include "ledmap.h"
    #include "layer.h"

// Typedefs:

    typedef enum {
        UiStyle_Classic,
        UiStyle_Alternative,
    } ui_style_t;

    extern ui_style_t UiStyle;

    typedef struct {
        // modules
        module_configuration_t ModuleConfigurations[ModuleId_RealModuleCount];
        caret_configuration_t NavigationModes[NavigationMode_RemappableCount];
        navigation_mode_t TouchpadPinchZoomMode;
        uint16_t HoldContinuationTimeout;

        // secondary roles
        secondary_role_strategy_t SecondaryRoles_Strategy;
        uint16_t SecondaryRoles_AdvancedStrategyDoubletapTimeout;
        uint16_t SecondaryRoles_AdvancedStrategyTimeout;
        int16_t SecondaryRoles_AdvancedStrategySafetyMargin;
        bool SecondaryRoles_AdvancedStrategyTriggerByRelease;
        bool SecondaryRoles_AdvancedStrategyTriggerByPress;
        bool SecondaryRoles_AdvancedStrategyTriggerByMouse;
        bool SecondaryRoles_AdvancedStrategyDoubletapToPrimary;
        secondary_role_state_t SecondaryRoles_AdvancedStrategyTimeoutAction;

        // mouse keys
        mouse_kinetic_state_t MouseMoveState;
        mouse_kinetic_state_t MouseScrollState;
        bool DiagonalSpeedCompensation;

        // key behavior
        uint16_t KeystrokeDelay;
        uint16_t DoubletapTimeout;
        uint16_t HoldTimeout; // not present in UserConfig atm.
        uint16_t DoubletapSwitchLayerReleaseTimeout;
        sticky_strategy_t StickyModifierStrategy;
        uint8_t DebounceTimePress;
        uint8_t DebounceTimeRelease;

        // macro engine
        uint8_t Macros_MaxBatchSize;
        macro_scheduler_t Macros_Scheduler;

        // backlight
        rgb_t KeyActionColors[keyActionColor_Length];
        float LedBrightnessMultiplier;
        rgb_t LedMap_ConstantRGB;
        backlighting_mode_t BacklightingMode;
        uint8_t DisplayBrightnessDefault;
        uint8_t DisplayBrightnessBatteryDefault;
        uint8_t KeyBacklightBrightnessDefault;
        uint8_t KeyBacklightBrightnessBatteryDefault;
        uint8_t KeyBacklightBrightnessChargingPercent;

        uint32_t DisplayFadeOutTimeout;
        uint32_t DisplayFadeOutBatteryTimeout;
        uint32_t KeyBacklightFadeOutTimeout;
        uint32_t KeyBacklightFadeOutBatteryTimeout;

        bool LedsEnabled;
        bool LedsAlwaysOn;

        // battery
        bool BatteryStationaryMode;

        // layers
        layer_config_t LayerConfig[LayerId_Count];

        // advanced
        uint16_t AutoRepeatInitialDelay;
        uint16_t AutoRepeatDelayRate;
        uint16_t Macros_OneShotTimeout;
        uint16_t AutoShiftDelay;
        uint8_t ChordingDelay;
        key_state_t* EmergencyKey;

        // others
        uint32_t I2cBaudRate;
        bool Bt_AllowUnsecuredConnections;
        bool Bt_DirectedAdvertisingAllowed;
        bool Bt_Enabled;
        bool Bt_AlwaysAdvertiseHid;
        uint8_t Bt_MaxPeripheralConnections;
        ui_style_t UiStyle;
    } config_t;

// Variables:

    extern config_t Cfg;

// Functions:

    void ConfigManager_ResetConfiguration(bool updateLeds);

#endif
