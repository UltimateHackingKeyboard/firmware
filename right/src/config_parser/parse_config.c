#include <string.h>
#include "basic_types.h"
#include "macros/status_buffer.h"
#include "parse_config.h"
#include "parse_keymap.h"
#include "parse_macro.h"
#include "keymap.h"
#include "config_globals.h"
#include "macros/core.h"
#include "led_display.h"
#include "ledmap.h"
#include "slave_scheduler.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "mouse_controller.h"
#include "mouse_keys.h"
#include "layer.h"
#include <stdint.h>
#include "parse_module_config.h"
#include "layer_switcher.h"
#include "usb_report_updater.h"
#include "debug.h"
#include "config_manager.h"
#include "led_manager.h"
#include "attributes.h"
#include "parse_host_connection.h"
#include "error_reporting.h"
#include "versioning.h"
#include "stubs.h"

version_t DataModelVersion = {0, 0, 0};

bool ConfigParser_ConfigVersionIsEmpty = true;

    bool PerKeyRgbPresent = false;

void readRgbColor(config_buffer_t *buffer, rgb_t* keyActionColors, key_action_color_t keyActionColor)
{
    rgb_t *color = &keyActionColors[keyActionColor];
    color->red = ReadUInt8(buffer);
    color->green = ReadUInt8(buffer);
    color->blue = ReadUInt8(buffer);
}


parser_error_t parseConfig(config_buffer_t *buffer)
{
    // Miscellaneous properties

    uint16_t len;
    uint16_t macroCount;
    uint16_t keymapCount;
    parser_error_t errorCode;

    DataModelVersion.major = ReadUInt16(buffer);
    DataModelVersion.minor = ReadUInt16(buffer);
    DataModelVersion.patch = ReadUInt16(buffer);

    ConfigParser_ConfigVersionIsEmpty = VERSION_EQUAL(DataModelVersion, 0, 0, 0) || VERSION_EQUAL(DataModelVersion, 0xFFFF, 0xFFFF, 0xFFFF);

#ifdef __ZEPHYR__
    if (!ParserRunDry) {
        printk("Flashed User Config version: %u.%u.%u\n", DataModelVersion.major, DataModelVersion.minor, DataModelVersion.patch);
    }
#endif

    uint32_t userConfigLength = DataModelVersion.major < 6 ? ReadUInt16(buffer) : ReadUInt32(buffer);
    const char *deviceName = ReadString(buffer, &len);
    uint16_t doubleTapSwitchLayerTimeout = ReadUInt16(buffer);

    (void)deviceName;
    (void)doubleTapSwitchLayerTimeout;

    // LED brightness

    ATTR_UNUSED uint8_t iconsAndLayerTextsBrightness = 0xff;
    uint8_t alphanumericSegmentsBrightness = 0xff;
    uint8_t keyBacklightBrightness = 0xff;

    if (DataModelVersion.major < 8) {
        iconsAndLayerTextsBrightness = ReadUInt8(buffer);
        alphanumericSegmentsBrightness = ReadUInt8(buffer);
        keyBacklightBrightness = ReadUInt8(buffer);
    }

    uint32_t ledsFadeTimeout = Cfg.DisplayFadeOutTimeout;
    bool previousPerKeyRgbPresent = PerKeyRgbPresent;
    backlighting_mode_t backlightingMode = Cfg.BacklightingMode;
    rgb_t keyActionColors[keyActionColor_Length];

    if (DataModelVersion.major >= 6) {
        if (DataModelVersion.major < 8) {
            ledsFadeTimeout = 1000 * ReadUInt16(buffer);
        }
        PerKeyRgbPresent = ReadBool(buffer);
        backlightingMode = ReadUInt8(buffer);

        readRgbColor(buffer, keyActionColors, KeyActionColor_None);
        readRgbColor(buffer, keyActionColors, KeyActionColor_Scancode);
        readRgbColor(buffer, keyActionColors, KeyActionColor_Modifier);
        readRgbColor(buffer, keyActionColors, KeyActionColor_Shortcut);
        readRgbColor(buffer, keyActionColors, KeyActionColor_SwitchLayer);
        readRgbColor(buffer, keyActionColors, KeyActionColor_SwitchKeymap);
        readRgbColor(buffer, keyActionColors, KeyActionColor_Mouse);
        readRgbColor(buffer, keyActionColors, KeyActionColor_Macro);
    }

    // Mouse kinetic properties

    uint8_t mouseMoveInitialSpeed = ReadUInt8(buffer);
    uint8_t mouseMoveAcceleration = ReadUInt8(buffer);
    uint8_t mouseMoveDeceleratedSpeed = ReadUInt8(buffer);
    uint8_t mouseMoveBaseSpeed = ReadUInt8(buffer);
    uint8_t mouseMoveAcceleratedSpeed = ReadUInt8(buffer);
    uint8_t mouseScrollInitialSpeed = ReadUInt8(buffer);
    uint8_t mouseScrollAcceleration = ReadUInt8(buffer);
    uint8_t mouseScrollDeceleratedSpeed = ReadUInt8(buffer);
    uint8_t mouseScrollBaseSpeed = ReadUInt8(buffer);
    uint8_t mouseScrollAcceleratedSpeed = ReadUInt8(buffer);

    if (mouseMoveInitialSpeed == 0 ||
        mouseMoveAcceleration == 0 ||
        mouseMoveDeceleratedSpeed == 0 ||
        mouseMoveBaseSpeed == 0 ||
        mouseMoveAcceleratedSpeed == 0 ||
        mouseScrollInitialSpeed == 0 ||
        mouseScrollAcceleration == 0 ||
        mouseScrollDeceleratedSpeed == 0 ||
        mouseScrollBaseSpeed == 0 ||
        mouseScrollAcceleratedSpeed == 0)
    {
        ConfigParser_Error(buffer, "Invalid mouse kinetic property");
        return ParserError_InvalidMouseKineticProperty;
    }

    // Version 7:

    float mouseMoveAxisSkew = 1.0f;
    float mouseScrollAxisSkew = 1.0f;
    bool diagonalSpeedCompensation = false;

    uint16_t doubletapTimeout = Cfg.DoubletapTimeout;
    uint16_t keystrokeDelay = Cfg.KeystrokeDelay;

    secondary_role_strategy_t secondaryRoles_Strategy = SecondaryRoleStrategy_Simple;
    uint16_t secondaryRoles_AdvancedStrategyDoubletapTimeout = Cfg.SecondaryRoles_AdvancedStrategyDoubletapTimeout;
    uint16_t secondaryRoles_AdvancedStrategyTimeout = Cfg.SecondaryRoles_AdvancedStrategyTimeout;
    int16_t secondaryRoles_AdvancedStrategySafetyMargin = Cfg.SecondaryRoles_AdvancedStrategySafetyMargin;
    bool secondaryRoles_AdvancedStrategyTriggerByRelease = Cfg.SecondaryRoles_AdvancedStrategyTriggerByRelease;
    bool secondaryRoles_AdvancedStrategyDoubletapToPrimary = Cfg.SecondaryRoles_AdvancedStrategyDoubletapToPrimary;
    serialized_secondary_role_action_type_t secondaryRoles_AdvancedStrategyTimeoutAction = SerializedSecondaryRoleActionType_Secondary;

    if (DataModelVersion.major >= 7) {
        secondaryRoles_Strategy = ReadUInt8(buffer);
        secondaryRoles_AdvancedStrategyDoubletapTimeout = ReadUInt16(buffer);
        secondaryRoles_AdvancedStrategyTimeout = ReadUInt16(buffer);
        secondaryRoles_AdvancedStrategySafetyMargin = ReadInt16(buffer);
        secondaryRoles_AdvancedStrategyTriggerByRelease = ReadBool(buffer);
        secondaryRoles_AdvancedStrategyDoubletapToPrimary = ReadBool(buffer);
        secondaryRoles_AdvancedStrategyTimeoutAction = ReadUInt8(buffer);

        mouseScrollAxisSkew = ReadFloat(buffer);
        mouseMoveAxisSkew = ReadFloat(buffer);
        diagonalSpeedCompensation = ReadBool(buffer);

        doubletapTimeout = ReadUInt16(buffer);
        keystrokeDelay = ReadUInt16(buffer);
    }

    // Version 8:

    uint8_t displayBrightness;
    uint8_t displayBrightnessBattery;
    uint8_t keyBacklightBrightnessBattery;

    uint32_t displayFadeOutTimeout;
    uint32_t displayFadeOutBatteryTimeout;
    uint32_t keyBacklightFadeOutTimeout;
    uint32_t keyBacklightFadeOutBatteryTimeout;

    if (DataModelVersion.major >= 8) {
        displayBrightness = ReadUInt8(buffer);
        displayBrightnessBattery = ReadUInt8(buffer);
        keyBacklightBrightness = ReadUInt8(buffer);
        keyBacklightBrightnessBattery = ReadUInt8(buffer);
        displayFadeOutTimeout = 1000 * ReadUInt16(buffer);
        displayFadeOutBatteryTimeout = 1000 * ReadUInt16(buffer);
        keyBacklightFadeOutTimeout = 1000 * ReadUInt16(buffer);
        keyBacklightFadeOutBatteryTimeout = 1000 * ReadUInt16(buffer);
    } else {
        displayBrightness = alphanumericSegmentsBrightness;
        displayBrightnessBattery = alphanumericSegmentsBrightness;
        keyBacklightBrightness = keyBacklightBrightness;
        keyBacklightBrightnessBattery = keyBacklightBrightness;
        displayFadeOutTimeout = ledsFadeTimeout;
        displayFadeOutBatteryTimeout = ledsFadeTimeout;
        keyBacklightFadeOutTimeout = ledsFadeTimeout;
        keyBacklightFadeOutBatteryTimeout = ledsFadeTimeout;
    }

    // HostConnection configuration

    if (VERSION_AT_LEAST(DataModelVersion, 8, 1, 0)) {
        RETURN_ON_ERROR(
            ParseHostConnections(buffer);
        )
    }


    // Module configurations

    uint16_t moduleConfigurationCount = ReadCompactLength(buffer);

    if (moduleConfigurationCount > 255) {
        ConfigParser_Error(buffer, "Invalid module configuration count: %u", moduleConfigurationCount);
        return ParserError_InvalidModuleConfigurationCount;
    }

    for (uint8_t moduleConfigurationIdx = 0; moduleConfigurationIdx < moduleConfigurationCount; moduleConfigurationIdx++) {
        RETURN_ON_ERROR(
            ParseModuleConfiguration(buffer);
        )
    }

    // Macros

    macroCount = ReadCompactLength(buffer);
    if (macroCount > MacroIndex_MaxUserDefinableCount) {
        ConfigParser_Error(buffer, "Too many macros: %u", macroCount);
        return ParserError_InvalidMacroCount;
    }

    for (uint8_t macroIdx = 0; macroIdx < macroCount; macroIdx++) {
        RETURN_ON_ERROR(
            ParseMacro(buffer, macroIdx);
        )
    }

    // Keymaps
    //
    keymapCount = ReadCompactLength(buffer);
    if (keymapCount == 0 || keymapCount > MAX_KEYMAP_NUM) {
        ConfigParser_Error(buffer, "Invalid keymap count: %u", keymapCount);
        return ParserError_InvalidKeymapCount;
    }

    parse_config_t parseConfig = (parse_config_t) {
        .mode = ParserRunDry ? ParseKeymapMode_DryRun : ParseKeymapMode_FullRun
    };

    for (uint8_t keymapIdx = 0; keymapIdx < keymapCount; keymapIdx++) {
        RETURN_ON_ERROR(
            ParseKeymap(buffer, keymapIdx, keymapCount, macroCount, parseConfig);
        )
    }

    // If parsing succeeded then apply the parsed values.

    if (!ParserRunDry) {
        // Update LED brightnesses and reinitialize LED drivers

        ValidatedUserConfigLength = userConfigLength;

        // removed in version 8
        // Cfg.IconsAndLayerTextsBrightnessDefault = iconsAndLayerTextsBrightness;
        // Cfg.AlphanumericSegmentsBrightnessDefault = alphanumericSegmentsBrightness;


        // Update mouse key speeds

        Cfg.MouseMoveState.initialSpeed = mouseMoveInitialSpeed;
        Cfg.MouseMoveState.acceleration = mouseMoveAcceleration;
        Cfg.MouseMoveState.deceleratedSpeed = mouseMoveDeceleratedSpeed;
        Cfg.MouseMoveState.baseSpeed = mouseMoveBaseSpeed;
        Cfg.MouseMoveState.acceleratedSpeed = mouseMoveAcceleratedSpeed;

        Cfg.MouseScrollState.initialSpeed = mouseScrollInitialSpeed;
        Cfg.MouseScrollState.acceleration = mouseScrollAcceleration;
        Cfg.MouseScrollState.deceleratedSpeed = mouseScrollDeceleratedSpeed;
        Cfg.MouseScrollState.baseSpeed = mouseScrollBaseSpeed;
        Cfg.MouseScrollState.acceleratedSpeed = mouseScrollAcceleratedSpeed;

        // Version 6

        if (DataModelVersion.major >= 6) {
            // removed in version 8
            // Cfg.LedsFadeTimeout = ledsFadeTimeout;

            Cfg.BacklightingMode = backlightingMode;

            memcpy(Cfg.KeyActionColors, keyActionColors, sizeof(keyActionColors));
        }

        // Version 7

        if (DataModelVersion.major >= 7) {
            Cfg.SecondaryRoles_Strategy = secondaryRoles_Strategy;
            Cfg.SecondaryRoles_AdvancedStrategyDoubletapTimeout = secondaryRoles_AdvancedStrategyDoubletapTimeout;
            Cfg.SecondaryRoles_AdvancedStrategyTimeout = secondaryRoles_AdvancedStrategyTimeout;
            Cfg.SecondaryRoles_AdvancedStrategySafetyMargin = secondaryRoles_AdvancedStrategySafetyMargin;
            Cfg.SecondaryRoles_AdvancedStrategyTriggerByRelease = secondaryRoles_AdvancedStrategyTriggerByRelease;
            Cfg.SecondaryRoles_AdvancedStrategyDoubletapToPrimary = secondaryRoles_AdvancedStrategyDoubletapToPrimary;
            switch (secondaryRoles_AdvancedStrategyTimeoutAction) {
                case SerializedSecondaryRoleActionType_Primary:
                    Cfg.SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Primary;
                    break;
                case SerializedSecondaryRoleActionType_Secondary:
                    Cfg.SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Secondary;
                    break;
                default:
                    ConfigParser_Error(buffer, "Invalid secondary role action type: %u", secondaryRoles_AdvancedStrategyTimeoutAction);
                    return ParserError_InvalidSecondaryRoleActionType;
            }

            Cfg.MouseMoveState.axisSkew = mouseMoveAxisSkew;
            Cfg.MouseScrollState.axisSkew = mouseScrollAxisSkew;
            Cfg.DiagonalSpeedCompensation = diagonalSpeedCompensation;

            Cfg.DoubletapTimeout = doubletapTimeout;
            Cfg.KeystrokeDelay = keystrokeDelay;
        }

        // Version 8


        Cfg.DisplayBrightnessDefault = displayBrightness;
        Cfg.DisplayBrightnessBatteryDefault = displayBrightnessBattery;
        Cfg.KeyBacklightBrightnessDefault = keyBacklightBrightness;
        Cfg.KeyBacklightBrightnessBatteryDefault = keyBacklightBrightnessBattery;
        Cfg.DisplayFadeOutTimeout = displayFadeOutTimeout;
        Cfg.DisplayFadeOutBatteryTimeout = displayFadeOutBatteryTimeout;
        Cfg.KeyBacklightFadeOutTimeout = keyBacklightFadeOutTimeout;
        Cfg.KeyBacklightFadeOutBatteryTimeout = keyBacklightFadeOutBatteryTimeout;

        AlwaysOnMode = false;
        LedManager_RecalculateLedBrightness();
        LedManager_UpdateSleepModes();
        BtPair_ClearUnknownBonds();

        // Update counts

        AllKeymapsCount = keymapCount;
        AllMacrosCount = macroCount;
    } else {
        PerKeyRgbPresent = previousPerKeyRgbPresent;
    }

    return ParserError_Success;
}


parser_error_t ParseConfig(config_buffer_t *buffer) {
    version_t oldModelVersion = DataModelVersion;
    parser_error_t errorCode = parseConfig(buffer);
    if (errorCode != ParserError_Success || ParserRunDry) {
        DataModelVersion = oldModelVersion;
    }
    return errorCode;
}
