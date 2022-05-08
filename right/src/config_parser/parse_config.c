#include "parse_config.h"
#include "parse_keymap.h"
#include "parse_macro.h"
#include "keymap.h"
#include "config_globals.h"
#include "macros.h"
#include "led_display.h"
#include "slave_scheduler.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "config.h"
#include "mouse_controller.h"

    uint16_t DataModelMajorVersion = 0;
    uint16_t DataModelMinorVersion = 0;
    uint16_t DataModelPatchVersion = 0;

static parser_error_t parseModuleConfiguration(config_buffer_t *buffer)
{
    uint8_t id = ReadUInt8(buffer);
    uint8_t pointerMode = ReadUInt8(buffer);  // move vs scroll
    uint8_t deceleratedPointerSpeedMultiplier = ReadUInt8(buffer);
    uint8_t basePointerSpeedMultiplier = ReadUInt8(buffer);
    uint8_t acceleratedPointerSpeed = ReadUInt8(buffer);
    uint16_t angularShift = ReadUInt16(buffer);
    uint8_t modLayerPointerFunction = ReadUInt8(buffer);  // none vs invertMode vs decelerate vs accelerate
    uint8_t fnLayerPointerFunction = ReadUInt8(buffer);  // none vs invertMode vs decelerate vs accelerate
    uint8_t mouseLayerPointerFunction = ReadUInt8(buffer);  // none vs invertMode vs decelerate vs accelerate

    (void)id;
    (void)pointerMode;
    (void)deceleratedPointerSpeedMultiplier;
    (void)basePointerSpeedMultiplier;
    (void)acceleratedPointerSpeed;
    (void)angularShift;
    (void)modLayerPointerFunction;
    (void)fnLayerPointerFunction;
    (void)mouseLayerPointerFunction;

    return ParserError_Success;
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
    uint16_t userConfigLength = ReadUInt16(buffer);
    const char *deviceName = ReadString(buffer, &len);
    uint16_t doubleTapSwitchLayerTimeout = ReadUInt16(buffer);

    (void)deviceName;
    (void)doubleTapSwitchLayerTimeout;

    // LED brightness

    uint8_t iconsAndLayerTextsBrightness = ReadUInt8(buffer);
    uint8_t alphanumericSegmentsBrightness = ReadUInt8(buffer);
    uint8_t keyBacklightBrightness = ReadUInt8(buffer);

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

    // Module configurations

    uint16_t moduleConfigurationCount = ReadCompactLength(buffer);

    if (moduleConfigurationCount > 255) {
        return ParserError_InvalidModuleConfigurationCount;
    }

    for (uint8_t moduleConfigurationIdx = 0; moduleConfigurationIdx < moduleConfigurationCount; moduleConfigurationIdx++) {
        errorCode = parseModuleConfiguration(buffer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }

    // Macros

    macroCount = ReadCompactLength(buffer);
    if (macroCount > MAX_MACRO_NUM) {
        return ParserError_InvalidMacroCount;
    }

    for (uint8_t macroIdx = 0; macroIdx < macroCount; macroIdx++) {
        errorCode = ParseMacro(buffer, macroIdx);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }

    // Keymaps

    keymapCount = ReadCompactLength(buffer);
    if (keymapCount == 0 || keymapCount > MAX_KEYMAP_NUM) {
        return ParserError_InvalidKeymapCount;
    }

    for (uint8_t keymapIdx = 0; keymapIdx < keymapCount; keymapIdx++) {
        errorCode = ParseKeymap(buffer, keymapIdx, keymapCount, macroCount);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }

    // If parsing succeeded then apply the parsed values.

    if (!ParserRunDry) {
//        DoubleTapSwitchLayerTimeout = doubleTapSwitchLayerTimeout;

        // Update LED brightnesses and reinitialize LED drivers

        ValidatedUserConfigLength = userConfigLength;

        IconsAndLayerTextsBrightnessDefault = iconsAndLayerTextsBrightness;
        AlphanumericSegmentsBrightnessDefault = alphanumericSegmentsBrightness;
        KeyBacklightBrightnessDefault = keyBacklightBrightness;

        LedSlaveDriver_UpdateLeds();

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

        // Update counts

        AllKeymapsCount = keymapCount;
        AllMacrosCount = macroCount;
    }

    return ParserError_Success;
}
