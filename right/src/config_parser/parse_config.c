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
#include "macros/vars.h"
#include "wormhole.h"

#if DEVICE_HAS_OLED
#include "keyboard/oled/widgets/widgets.h"
#endif


#ifdef __ZEPHYR__
#include "usb_commands/usb_command_get_new_pairings.h"
#include "bt_pair.h"
#include "state_sync.h"
#else
#include "segment_display.h"
#endif

version_t DataModelVersion = {0, 0, 0};

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

    if (VERSION_AT_LEAST(DataModelVersion, userConfigVersion.major, userConfigVersion.minor+1, 0)) {
        Macros_ReportErrorPrintf(NULL,
            "Config version too new: %u.%u.%u (firmware's userconfig: %u.%u.%u)\n",
            DataModelVersion.major, DataModelVersion.minor, DataModelVersion.patch,
            userConfigVersion.major, userConfigVersion.minor, userConfigVersion.patch
        );
        #ifndef __ZEPHYR__
            SegmentDisplay_SetText(3, "DOW", SegmentDisplaySlot_Error);
        #endif
        return ParserError_ConfigVersionTooNew;
    }

#ifdef __ZEPHYR__
    if (!ParserRunDry) {
        printk(
                "Flashed User Config version: %u.%u.%u (native version: %u.%u.%u., at %s / %s)\n",
                DataModelVersion.major, DataModelVersion.minor, DataModelVersion.patch,
                userConfigVersion.major, userConfigVersion.minor, userConfigVersion.patch,
                gitTag,
                DeviceMD5Checksums[DEVICE_ID]
        );
    }
#endif

    uint32_t userConfigLength = DataModelVersion.major < 6 ? ReadUInt16(buffer) : ReadUInt32(buffer);
    const char *deviceName = ReadString(buffer, &len);
    uint16_t doubleTapSwitchLayerTimeout = ReadUInt16(buffer);

    // Save device name as string_ref_t
    Cfg.DeviceName.offset = deviceName - (const char*)ValidatedUserConfigBuffer.buffer;
    Cfg.DeviceName.len = len;

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
        if (DataModelVersion.major >= 9) {
            readRgbColor(buffer, keyActionColors, KeyActionColor_Device);
        } else {
            keyActionColors[KeyActionColor_Device] = Cfg.KeyActionColors[KeyActionColor_Device];
        }
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
    uint16_t secondaryRoles_AdvancedStrategyTimeout = Cfg.SecondaryRoles_AdvancedStrategyTimeout;
    int16_t secondaryRoles_AdvancedStrategySafetyMargin = Cfg.SecondaryRoles_AdvancedStrategySafetyMargin;
    serialized_secondary_role_triggering_event_t secondaryRoles_AdvancedStrategyTriggeringEvent = SerializedSecondaryRoleTriggeringEvent_Release;
    bool secondaryRoles_AdvancedStrategyDoubletapToPrimary = Cfg.SecondaryRoles_AdvancedStrategyDoubletapToPrimary;
    serialized_secondary_role_action_type_t secondaryRoles_AdvancedStrategyTimeoutAction = SerializedSecondaryRoleActionType_Secondary;

    bool secondaryRoles_AdvancedStrategyTriggerByMouse = Cfg.SecondaryRoles_AdvancedStrategyTriggerByMouse;
    bool secondaryRoles_AdvancedStrategyAcceptTriggersFromSameHalf = Cfg.SecondaryRoles_AdvancedStrategyAcceptTriggersFromSameHalf;
    uint8_t secondaryRoles_AdvancedStrategyMinimumHoldTime = Cfg.SecondaryRoles_AdvancedStrategyMinimumHoldTime;
    serialized_secondary_role_timeout_type_t secondaryRoles_AdvancedStrategyTimeoutType = SerializedSecondaryRoleTimeoutType_Active;

    if (DataModelVersion.major >= 7) {
        secondaryRoles_Strategy = ReadUInt8(buffer);
        ATTR_UNUSED uint16_t secondaryRoles_AdvancedStrategyDoubletapTimeout = ReadUInt16(buffer);
        secondaryRoles_AdvancedStrategyTimeout = ReadUInt16(buffer);
        secondaryRoles_AdvancedStrategySafetyMargin = ReadInt16(buffer);
        secondaryRoles_AdvancedStrategyTriggeringEvent = ReadUInt8(buffer);
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

    // Version 10:

    uint8_t batteryChargingMode = SerializedChargingMode_Full;
    uint8_t keyBacklightBrightnessChargingPercent = 50;

    if (VERSION_AT_LEAST(DataModelVersion, 9, 99, 0)) {
        keyBacklightBrightnessChargingPercent = ReadUInt8(buffer);
        batteryChargingMode = ReadUInt8(buffer);
    }

    // Version 14:

    if (DataModelVersion.major >= 14) {
        secondaryRoles_AdvancedStrategyTriggerByMouse = ReadBool(buffer);
        secondaryRoles_AdvancedStrategyAcceptTriggersFromSameHalf = ReadBool(buffer);
        secondaryRoles_AdvancedStrategyMinimumHoldTime = ReadUInt8(buffer);
        secondaryRoles_AdvancedStrategyTimeoutType = ReadUInt8(buffer);
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

    if (DataModelVersion.major >= 13) {
        ATTR_UNUSED uint16_t l;
        ATTR_UNUSED const char *lastSaveAgentTag = ReadString(buffer, &len);
        ATTR_UNUSED const char *lastSaveFirmwareTag = ReadString(buffer, &len);
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
            Cfg.SecondaryRoles_AdvancedStrategyTimeout = secondaryRoles_AdvancedStrategyTimeout;
            Cfg.SecondaryRoles_AdvancedStrategySafetyMargin = secondaryRoles_AdvancedStrategySafetyMargin;
            switch (secondaryRoles_AdvancedStrategyTriggeringEvent) {
                case SerializedSecondaryRoleTriggeringEvent_Press:
                    Cfg.SecondaryRoles_AdvancedStrategyTriggeringEvent = SecondaryRoleTriggeringEvent_Press;
                    break;
                case SerializedSecondaryRoleTriggeringEvent_Release:
                    Cfg.SecondaryRoles_AdvancedStrategyTriggeringEvent = SecondaryRoleTriggeringEvent_Release;
                    break;
                case SerializedSecondaryRoleTriggeringEvent_None:
                    Cfg.SecondaryRoles_AdvancedStrategyTriggeringEvent = SecondaryRoleTriggeringEvent_None;
                    break;
                default:
                    ConfigParser_Error(buffer, "Invalid secondary role triggering event: %u", secondaryRoles_AdvancedStrategyTriggeringEvent);
                    return ParserError_InvalidSecondaryRoleTriggeringEvent;
            }
            Cfg.SecondaryRoles_AdvancedStrategyDoubletapToPrimary = secondaryRoles_AdvancedStrategyDoubletapToPrimary;
            switch (secondaryRoles_AdvancedStrategyTimeoutAction) {
                case SerializedSecondaryRoleActionType_Primary:
                    Cfg.SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Primary;
                    break;
                case SerializedSecondaryRoleActionType_Secondary:
                    Cfg.SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_Secondary;
                    break;
                case SerializedSecondaryRoleActionType_NoOp:
                    Cfg.SecondaryRoles_AdvancedStrategyTimeoutAction = SecondaryRoleState_NoOp;
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

        // Version 14

        if (DataModelVersion.major >= 14) {
            Cfg.SecondaryRoles_AdvancedStrategyTriggerByMouse = secondaryRoles_AdvancedStrategyTriggerByMouse;
            Cfg.SecondaryRoles_AdvancedStrategyAcceptTriggersFromSameHalf = secondaryRoles_AdvancedStrategyAcceptTriggersFromSameHalf;
            Cfg.SecondaryRoles_AdvancedStrategyMinimumHoldTime = secondaryRoles_AdvancedStrategyMinimumHoldTime;
            switch (secondaryRoles_AdvancedStrategyTimeoutType) {
                case SerializedSecondaryRoleTimeoutType_Active:
                    Cfg.SecondaryRoles_AdvancedStrategyTimeoutType = SecondaryRoleTimeoutType_Active;
                    break;
                case SerializedSecondaryRoleTimeoutType_Passive:
                    Cfg.SecondaryRoles_AdvancedStrategyTimeoutType = SecondaryRoleTimeoutType_Passive;
                    break;
                default:
                    ConfigParser_Error(buffer, "Invalid secondary role timeout type: %u", secondaryRoles_AdvancedStrategyTimeoutType);
                    return ParserError_InvalidSecondaryRoleTimeoutType;
            }
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

        // Version 10
        Cfg.KeyBacklightBrightnessChargingPercent = keyBacklightBrightnessChargingPercent;
        Cfg.BatteryStationaryMode = batteryChargingMode > SerializedChargingMode_Full ? SerializedChargingMode_StationaryMode : SerializedChargingMode_Full;

#ifdef __ZEPHYR__
        StateSync_UpdateProperty(StateSyncPropertyId_BatteryStationaryMode, &Cfg.BatteryStationaryMode);
        BtPair_AllocateUnregisteredBonds();
        BtConn_UpdateHostConnectionPeerAllocations();
        UsbCommand_UpdateNewPairingsFlag();
#endif
        WormCfg->devMode = Cfg.DevMode;
        LedManager_FullUpdate();
        MacroVariables_Reset();
        WIDGET_REFRESH(&TargetWidget); // the target may have been renamed

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
