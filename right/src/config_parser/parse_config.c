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

    uint16_t DataModelMajorVersion = 0;
    uint16_t DataModelMinorVersion = 0;
    uint16_t DataModelPatchVersion = 0;

    bool PerKeyRgbPresent = false;

void readRgbColor(config_buffer_t *buffer, rgb_t* keyActionColors, key_action_color_t keyActionColor)
{
    rgb_t *color = &keyActionColors[keyActionColor];
    color->red = ReadUInt8(buffer);
    color->green = ReadUInt8(buffer);
    color->blue = ReadUInt8(buffer);
}

parser_error_t ParseConfig(config_buffer_t *buffer)
{
    // Miscellaneous properties

    uint16_t len;
    uint16_t macroCount;
    uint16_t keymapCount;
    parser_error_t errorCode;

    DataModelMajorVersion = ReadUInt16(buffer);
    DataModelMinorVersion = ReadUInt16(buffer);
    DataModelPatchVersion = ReadUInt16(buffer);
    uint32_t userConfigLength = DataModelMajorVersion < 6 ? ReadUInt16(buffer) : ReadUInt32(buffer);
    const char *deviceName = ReadString(buffer, &len);
    uint16_t doubleTapSwitchLayerTimeout = ReadUInt16(buffer);

    (void)deviceName;
    (void)doubleTapSwitchLayerTimeout;

    // LED brightness

    uint8_t iconsAndLayerTextsBrightness = ReadUInt8(buffer);
    uint8_t alphanumericSegmentsBrightness = ReadUInt8(buffer);
    uint8_t keyBacklightBrightness = ReadUInt8(buffer);

    uint32_t ledsFadeTimeout = LedsFadeTimeout;
    bool previousPerKeyRgbPresent = PerKeyRgbPresent;
    backlighting_mode_t backlightingMode = BacklightingMode;
    rgb_t keyActionColors[keyActionColor_Length];

    if (DataModelMajorVersion >= 6) {
        ledsFadeTimeout = 1000 * ReadUInt16(buffer);
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
        return ParserError_InvalidMouseKineticProperty;
    }

    // Version 7:

    float mouseMoveAxisSkew = 1.0f;
    float mouseScrollAxisSkew = 1.0f;
    bool diagonalSpeedCompensation = false;

    uint16_t doubletapTimeout = DoubletapTimeout;
    uint16_t keystrokeDelay = KeystrokeDelay;

    secondary_role_strategy_t secondaryRoles_Strategy = SecondaryRoleStrategy_Simple;
    uint16_t secondaryRoles_AdvancedStrategyDoubletapTimeout = SecondaryRoles_AdvancedStrategyDoubletapTimeout;
    uint16_t secondaryRoles_AdvancedStrategyTimeout = SecondaryRoles_AdvancedStrategyTimeout;
    int16_t secondaryRoles_AdvancedStrategySafetyMargin = SecondaryRoles_AdvancedStrategySafetyMargin;
    bool secondaryRoles_AdvancedStrategyTriggerByRelease = SecondaryRoles_AdvancedStrategyTriggerByRelease;
    bool secondaryRoles_AdvancedStrategyDoubletapToPrimary = SecondaryRoles_AdvancedStrategyDoubletapToPrimary;
    serialized_secondary_role_action_type_t secondaryRoles_AdvancedStrategyTimeoutAction = SerializedSecondaryRoleActionType_Secondary;

    if (DataModelMajorVersion >= 7) {
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

    // Module configurations

    uint16_t moduleConfigurationCount = ReadCompactLength(buffer);

    if (moduleConfigurationCount > 255) {
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

        IconsAndLayerTextsBrightnessDefault = iconsAndLayerTextsBrightness;
        AlphanumericSegmentsBrightnessDefault = alphanumericSegmentsBrightness;
        KeyBacklightBrightnessDefault = keyBacklightBrightness;

        LedSlaveDriver_RecalculateLedBrightness();

        // Update mouse key speeds

        MouseMoveState.initialSpeed = mouseMoveInitialSpeed;
        MouseMoveState.acceleration = mouseMoveAcceleration;
        MouseMoveState.deceleratedSpeed = mouseMoveDeceleratedSpeed;
        MouseMoveState.baseSpeed = mouseMoveBaseSpeed;
        MouseMoveState.acceleratedSpeed = mouseMoveAcceleratedSpeed;

        MouseScrollState.initialSpeed = mouseScrollInitialSpeed;
        MouseScrollState.acceleration = mouseScrollAcceleration;
        MouseScrollState.deceleratedSpeed = mouseScrollDeceleratedSpeed;
        MouseScrollState.baseSpeed = mouseScrollBaseSpeed;
        MouseScrollState.acceleratedSpeed = mouseScrollAcceleratedSpeed;

        // Version 6

        if (DataModelMajorVersion >= 6) {
            LedsFadeTimeout = ledsFadeTimeout;
            BacklightingMode = backlightingMode;

            memcpy(KeyActionColors, keyActionColors, sizeof(keyActionColors));
        }

        // Version 7

        if (DataModelMajorVersion >= 7) {
            SecondaryRoles_Strategy = secondaryRoles_Strategy;
            SecondaryRoles_AdvancedStrategyDoubletapTimeout = secondaryRoles_AdvancedStrategyDoubletapTimeout;
            SecondaryRoles_AdvancedStrategyTimeout = secondaryRoles_AdvancedStrategyTimeout;
            SecondaryRoles_AdvancedStrategySafetyMargin = secondaryRoles_AdvancedStrategySafetyMargin;
            SecondaryRoles_AdvancedStrategyTriggerByRelease = secondaryRoles_AdvancedStrategyTriggerByRelease;
            SecondaryRoles_AdvancedStrategyDoubletapToPrimary = secondaryRoles_AdvancedStrategyDoubletapToPrimary;
            switch (secondaryRoles_AdvancedStrategyTimeoutAction) {
                case SerializedSecondaryRoleActionType_Primary:
                    SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Primary;
                    break;
                case SerializedSecondaryRoleActionType_Secondary:
                    SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Secondary;
                    break;
                default:
                    return ParserError_InvalidSecondaryRoleActionType;
            }

            MouseMoveState.axisSkew = mouseMoveAxisSkew;
            MouseScrollState.axisSkew = mouseScrollAxisSkew;
            DiagonalSpeedCompensation = diagonalSpeedCompensation;

            DoubletapTimeout = doubletapTimeout;
            KeystrokeDelay = keystrokeDelay;
        }

        // Update counts

        AllKeymapsCount = keymapCount;
        AllMacrosCount = macroCount;
    } else {
        PerKeyRgbPresent = previousPerKeyRgbPresent;
    }

    return ParserError_Success;
}
