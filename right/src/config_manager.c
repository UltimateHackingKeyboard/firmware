#include "config_manager.h"
#include <string.h>

#ifndef __ZEPHYR__
#include "i2c.h"
#include "init_peripherals.h"
#endif

const config_t DefaultCfg = (config_t){
    .ModuleConfigurations = {
        { // ModuleId_KeyClusterLeft
            .speed = 0.0,
            .baseSpeed = 5.0,
            .xceleration = 0.0,
            .scrollSpeedDivisor = 5.0f,
            .caretSpeedDivisor = 5.0f,
            .pinchZoomSpeedDivisor = 5.0f,
            .axisLockSkew = 0.5f,
            .axisLockFirstTickSkew = 0.5f,
            .cursorAxisLock = false,
            .scrollAxisLock = true,
            .caretAxisLock = true,
            .swapAxes = false,
            .invertScrollDirectionX = false,
            .invertScrollDirectionY = false,
            .navigationModes = {
                NavigationMode_Scroll, // Base layer
                NavigationMode_Cursor, // Mod layer
                NavigationMode_Caret, // Fn layer
                NavigationMode_Scroll, // Mouse layer
            }
        },
        { // ModuleId_TrackballRight
            .speed = 0.5, // min:0.2, opt:1.0/0.5, max:5
            .baseSpeed = 0.5, // min: 0, opt:0.0/0.5, max 5
            .xceleration = 1.0, // min:0.0, opt:0.5/1.0, max:2.0
            .scrollSpeedDivisor = 8.0f,
            .caretSpeedDivisor = 16.0f,
            .pinchZoomSpeedDivisor = 4.0f,
            .axisLockSkew = 0.5f,
            .axisLockFirstTickSkew = 2.0f,
            .cursorAxisLock = false,
            .scrollAxisLock = true,
            .caretAxisLock = true,
            .swapAxes = false,
            .invertScrollDirectionX = false,
            .invertScrollDirectionY = false,
            .navigationModes = {
                NavigationMode_Cursor, // Base layer
                NavigationMode_Scroll, // Mod layer
                NavigationMode_Caret, // Fn layer
                NavigationMode_Cursor, // Mouse layer
            }
        },
        { // ModuleId_TrackpointRight
            .speed = 1.0, // min:0.2, opt:1.0, max:5
            .baseSpeed = 0.0, // min: 0, opt = 0.0, max 5
            .xceleration = 0.0, // min:0.0, opt:0.0, max:2.0
            .scrollSpeedDivisor = 8.0f,
            .caretSpeedDivisor = 16.0f,
            .pinchZoomSpeedDivisor = 4.0f,
            .axisLockSkew = 0.5f,
            .axisLockFirstTickSkew = 2.0f,
            .cursorAxisLock = false,
            .scrollAxisLock = true,
            .caretAxisLock = true,
            .swapAxes = false,
            .invertScrollDirectionX = false,
            .invertScrollDirectionY = false,
            .navigationModes = {
                NavigationMode_Cursor, // Base layer
                NavigationMode_Scroll, // Mod layer
                NavigationMode_Caret, // Fn layer
                NavigationMode_Cursor, // Mouse layer
            }
        },
        { // ModuleId_TouchpadRight
            .speed = 0.7, // min:0.2, opt:1.3/0.6, max:1.8
            .baseSpeed = 0.5, // min: 0, opt = 0.0/0.5, max 5
            .xceleration = 1.0, // min:0.0, opt:0.5/1.0, max:2.0
            .scrollSpeedDivisor = 8.0f,
            .caretSpeedDivisor = 16.0f,
            .pinchZoomSpeedDivisor = 4.0f,
            .axisLockSkew = 0.5f,
            .axisLockFirstTickSkew = 2.0f,
            .cursorAxisLock = false,
            .scrollAxisLock = true,
            .caretAxisLock = true,
            .swapAxes = false,
            .invertScrollDirectionX = false,
            .invertScrollDirectionY = false,
            .navigationModes = {
                NavigationMode_Cursor, // Base layer
                NavigationMode_Scroll, // Mod layer
                NavigationMode_Caret, // Fn layer
                NavigationMode_Cursor, // Mouse layer
            }
        },
        { // ModuleId_Next
            .speed = 1.0, // min:0.2, opt:1.0, max:5
            .baseSpeed = 0.5, // min: 0, opt = 0.0/0.5, max 5
            .xceleration = 5.0, // min:0.1, opt:5.0, max:10.0
            .scrollSpeedDivisor = 8.0f,
            .caretSpeedDivisor = 16.0f,
            .pinchZoomSpeedDivisor = 4.0f,
            .axisLockSkew = 0.5f,
            .axisLockFirstTickSkew = 1.0f,
            .cursorAxisLock = false,
            .scrollAxisLock = false,
            .caretAxisLock = true,
            .swapAxes = false,
            .invertScrollDirectionX = false,
            .invertScrollDirectionY = false,
            .navigationModes = {
                NavigationMode_Cursor, // Base layer
                NavigationMode_Scroll, // Mod layer
                NavigationMode_Caret, // Fn layer
                NavigationMode_Cursor, // Mouse layer
            }
        }
    },
        .MouseMoveState = {
            .isScroll = false,
            .upState = SerializedMouseAction_MoveUp,
            .downState = SerializedMouseAction_MoveDown,
            .leftState = SerializedMouseAction_MoveLeft,
            .rightState = SerializedMouseAction_MoveRight,
            .verticalStateSign = 0,
            .horizontalStateSign = 0,
            .intMultiplier = 25,
            .initialSpeed = 5,
            .acceleration = 35,
            .deceleratedSpeed = 10,
            .baseSpeed = 40,
            .acceleratedSpeed = 80,
            .axisSkew = 1.0f,
        },

        .MouseScrollState = {
            .isScroll = true,
            .upState = SerializedMouseAction_ScrollDown,
            .downState = SerializedMouseAction_ScrollUp,
            .leftState = SerializedMouseAction_ScrollLeft,
            .rightState = SerializedMouseAction_ScrollRight,
            .verticalStateSign = 0,
            .horizontalStateSign = 0,
            .intMultiplier = 1,
            .initialSpeed = 20,
            .acceleration = 20,
            .deceleratedSpeed = 10,
            .baseSpeed = 20,
            .acceleratedSpeed = 50,
            .axisSkew = 1.0f,
        },
        .DiagonalSpeedCompensation = false,
        .TouchpadPinchZoomMode = NavigationMode_Zoom,
        .HoldContinuationTimeout = 0,
        .SecondaryRoles_AdvancedStrategyDoubletapTimeout = 200,
        .SecondaryRoles_AdvancedStrategyTimeout = 350,
        .SecondaryRoles_AdvancedStrategySafetyMargin = 50,
        .SecondaryRoles_AdvancedStrategyTriggerByRelease = true,
        .SecondaryRoles_AdvancedStrategyTriggerByPress = false,
        .SecondaryRoles_AdvancedStrategyTriggerByMouse = false,
        .SecondaryRoles_AdvancedStrategyDoubletapToPrimary = true,
        .SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Secondary,
        .SecondaryRoles_Strategy = SecondaryRoleStrategy_Simple,
        .StickyModifierStrategy = Stick_Smart,
        .Macros_Scheduler = Scheduler_Blocking,
        .Macros_MaxBatchSize = 20,
        .LedMap_ConstantRGB = { 0xFF, 0xFF, 0xFF },
        .BacklightingMode = BacklightingMode_Functional,
        .DisplayBrightnessBatteryDefault = 0x20,
        .DisplayBrightnessDefault = 0xff,
        .KeyBacklightBrightnessBatteryDefault = 0x20,
        .KeyBacklightBrightnessDefault = 0xff,
        .DisplayFadeOutTimeout = 0,
        .DisplayFadeOutBatteryTimeout = 60000,
        .KeyBacklightFadeOutTimeout = 0,
        .KeyBacklightFadeOutBatteryTimeout = 60000,
        .LedsEnabled = true,
        .LedBrightnessMultiplier = 1.0f,
        .LayerConfig = {
            { .layerIsDefined = true, .exactModifierMatch = false, .modifierLayerMask = 0},
            { .layerIsDefined = true, .exactModifierMatch = false, .modifierLayerMask = 0},
            { .layerIsDefined = true, .exactModifierMatch = false, .modifierLayerMask = 0},
            { .layerIsDefined = true, .exactModifierMatch = false, .modifierLayerMask = 0},

            // fn2 - fn5
            { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = 0},
            { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = 0},
            { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = 0},
            { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = 0},

            // Shift, Control, Alt, Super
            { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = HID_KEYBOARD_MODIFIER_LEFTSHIFT | HID_KEYBOARD_MODIFIER_RIGHTSHIFT },
            { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_RIGHTCTRL },
            { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = HID_KEYBOARD_MODIFIER_LEFTALT | HID_KEYBOARD_MODIFIER_RIGHTALT },
            { .layerIsDefined = false, .exactModifierMatch = false, .modifierLayerMask = HID_KEYBOARD_MODIFIER_LEFTGUI | HID_KEYBOARD_MODIFIER_RIGHTGUI },
        },

        .DebounceTimePress = 50,
        .DebounceTimeRelease = 50,
        .DoubletapTimeout = 400,
        .DoubletapSwitchLayerReleaseTimeout = 200,
        .KeystrokeDelay = 0,
        .AutoRepeatInitialDelay = 500,
        .AutoRepeatDelayRate = 50,
        .Macros_OneShotTimeout = 500,
        .AutoShiftDelay = 0,
        .ChordingDelay = 0,
#ifdef __ZEPHYR__
        .I2cBaudRate = 0,
#else
        .I2cBaudRate = I2C_MAIN_BUS_NORMAL_BAUD_RATE,
#endif
        .EmergencyKey = NULL,
        .KeyActionColors = {
            {0x00, 0x00, 0x00}, // KeyActionColor_None
            {0xFF, 0xFF, 0xFF}, // KeyActionColor_Scancode
            {0x00, 0xFF, 0xFF}, // KeyActionColor_Modifier
            {0x00, 0x00, 0xFF}, // KeyActionColor_Shortcut
            {0xFF, 0xFF, 0x00}, // KeyActionColor_SwitchLayer
            {0xFF, 0x00, 0x00}, // KeyActionColor_SwitchKeymap
            {0x00, 0xFF, 0x00}, // KeyActionColor_Mouse
            {0xFF, 0x00, 0xFF}, // KeyActionColor_Macro
        },
};

config_t Cfg = {};

void ConfigManager_ResetConfiguration(bool updateLeds) {
    memcpy(&Cfg, &DefaultCfg, sizeof(Cfg));
#ifndef __ZEPHYR__
    ChangeI2cBaudRate(Cfg.I2cBaudRate);
#endif
    if (updateLeds) {
        Ledmap_UpdateBacklightLeds();
    }
}
