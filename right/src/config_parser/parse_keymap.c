#include "config_parser/config_globals.h"
#include "config_parser/parse_config.h"
#include "config_parser/parse_keymap.h"
#include "key_action.h"
#include "keymaps.h"
#include "led_display.h"

static uint8_t tempKeymapCount;
static uint8_t tempMacroCount;

static parser_error_t parseNoneAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    keyAction->type = KeyActionType_None;
    return ParserError_Success;
}

static parser_error_t parseKeyStrokeAction(key_action_t *keyAction, uint8_t keyStrokeAction, config_buffer_t *buffer)
{
    uint8_t keystrokeType = (SERIALIZED_KEYSTROKE_TYPE_MASK_KEYSTROKE_TYPE & keyStrokeAction) >> SERIALIZED_KEYSTROKE_TYPE_OFFSET_KEYSTROKE_TYPE;

    keyAction->type = KeyActionType_Keystroke;
    switch (keystrokeType) {
        case SerializedKeystrokeType_Basic:
            keyAction->keystroke.keystrokeType = KeystrokeType_Basic;
            break;
        case SerializedKeystrokeType_ShortMedia:
        case SerializedKeystrokeType_LongMedia:
            keyAction->keystroke.keystrokeType = KeystrokeType_Media;
            break;
        case SerializedKeystrokeType_System:
            keyAction->keystroke.keystrokeType = KeystrokeType_System;
            break;
        default:
            return ParserError_InvalidSerializedKeystrokeType;
    }
    keyAction->keystroke.scancode = keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_SCANCODE
        ? keystrokeType == SerializedKeystrokeType_LongMedia ? readUInt16(buffer) : readUInt8(buffer)
        : 0;
    keyAction->keystroke.modifiers = keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_MODIFIERS
        ? readUInt8(buffer)
        : 0;
    keyAction->keystroke.longPressAction = keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_LONGPRESS
        ? readUInt8(buffer)
        : 0;
    return ParserError_Success;
}

static parser_error_t parseSwitchLayerAction(key_action_t *KeyAction, config_buffer_t *buffer)
{
    uint8_t layer = readUInt8(buffer) + 1;
    bool isToggle = readBool(buffer);

    KeyAction->type = KeyActionType_SwitchLayer;
    KeyAction->switchLayer.layer = layer;
    KeyAction->switchLayer.isToggle = isToggle;
    return ParserError_Success;
}

static parser_error_t parseSwitchKeymapAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    uint8_t keymapIndex = readUInt8(buffer);

    if (keymapIndex >= tempKeymapCount) {
        return ParserError_InvalidSerializedSwitchKeymapAction;
    }
    keyAction->type = KeyActionType_SwitchKeymap;
    keyAction->switchKeymap.keymapId = keymapIndex;
    return ParserError_Success;
}

static parser_error_t parsePlayMacroAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    uint8_t macroIndex = readUInt8(buffer);

    if (macroIndex >= tempMacroCount) {
        return ParserError_InvalidSerializedPlayMacroAction;
    }
    keyAction->type = KeyActionType_SwitchKeymap;
    keyAction->playMacro.macroId = macroIndex;
    return ParserError_Success;
}

static parser_error_t parseMouseAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    uint8_t mouseAction = readUInt8(buffer);

    keyAction->type = KeyActionType_Mouse;
    memset(&keyAction->mouse, 0, sizeof keyAction->mouse);
    switch (mouseAction) {
        case SerializedMouseAction_LeftClick:
            keyAction->mouse.buttonActions = MouseButton_Left;
            break;
        case SerializedMouseAction_MiddleClick:
            keyAction->mouse.buttonActions = MouseButton_Middle;
            break;
        case SerializedMouseAction_RightClick:
            keyAction->mouse.buttonActions = MouseButton_Right;
            break;
        case SerializedMouseAction_MoveUp:
            keyAction->mouse.moveActions = MouseMove_Up;
            break;
        case SerializedMouseAction_MoveDown:
            keyAction->mouse.moveActions = MouseMove_Down;
            break;
        case SerializedMouseAction_MoveLeft:
            keyAction->mouse.moveActions = MouseMove_Left;
            break;
        case SerializedMouseAction_MoveRight:
            keyAction->mouse.moveActions = MouseMove_Right;
            break;
        case SerializedMouseAction_ScrollUp:
            keyAction->mouse.scrollActions = MouseScroll_Up;
            break;
        case SerializedMouseAction_ScrollDown:
            keyAction->mouse.scrollActions = MouseScroll_Down;
            break;
        case SerializedMouseAction_ScrollLeft:
            keyAction->mouse.scrollActions = MouseScroll_Left;
            break;
        case SerializedMouseAction_ScrollRight:
            keyAction->mouse.scrollActions = MouseScroll_Right;
            break;
        case SerializedMouseAction_Accelerate:
            keyAction->mouse.moveActions = MouseMove_Accelerate;
            break;
        case SerializedMouseAction_Decelerate:
            keyAction->mouse.moveActions = MouseMove_Decelerate;
            break;
        default:
            return ParserError_InvalidSerializedMouseAction;
    }
    return ParserError_Success;
}

static parser_error_t parseKeyAction(key_action_t *keyAction, config_buffer_t *buffer)
{
    uint8_t keyActionType = readUInt8(buffer);

    switch (keyActionType) {
        case SerializedKeyActionType_None:
            return parseNoneAction(keyAction, buffer);
        case SerializedKeyActionType_KeyStroke ... SerializedKeyActionType_LastKeyStroke:
            return parseKeyStrokeAction(keyAction, keyActionType, buffer);
        case SerializedKeyActionType_SwitchLayer:
            return parseSwitchLayerAction(keyAction, buffer);
        case SerializedKeyActionType_SwitchKeymap:
            return parseSwitchKeymapAction(keyAction, buffer);
        case SerializedKeyActionType_Mouse:
            return parseMouseAction(keyAction, buffer);
        case SerializedKeyActionType_PlayMacro:
            return parsePlayMacroAction(keyAction, buffer);
    }
    return ParserError_InvalidSerializedKeyActionType;
}

static parser_error_t parseKeyActions(uint8_t targetLayer, config_buffer_t *buffer, uint8_t moduleId, uint8_t pointerRole)
{
    parser_error_t errorCode;
    uint16_t actionCount = readCompactLength(buffer);
    key_action_t dummyKeyAction;

    if (actionCount > MAX_KEY_COUNT_PER_MODULE) {
        return ParserError_InvalidActionCount;
    }
    for (uint8_t actionIdx = 0; actionIdx < actionCount; actionIdx++) {
        errorCode = parseKeyAction(ParserRunDry ? &dummyKeyAction : &CurrentKeymap[targetLayer][moduleId][actionIdx], buffer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

static parser_error_t parseModule(config_buffer_t *buffer, uint8_t layer)
{
    uint8_t moduleId = readUInt8(buffer);
    uint8_t pointerRole = readUInt8(buffer);

    return parseKeyActions(layer, buffer, moduleId, pointerRole);
}

static parser_error_t parseLayer(config_buffer_t *buffer, uint8_t layer)
{
    parser_error_t errorCode;
    uint16_t moduleCount = readCompactLength(buffer);

    if (moduleCount > SLOT_COUNT) {
        return ParserError_InvalidModuleCount;
    }
    for (uint8_t moduleIdx = 0; moduleIdx < moduleCount; moduleIdx++) {
        errorCode = parseModule(buffer, layer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

parser_error_t ParseKeymap(config_buffer_t *buffer, uint8_t keymapIdx, uint8_t keymapCount, uint8_t macroCount)
{
    uint16_t offset = buffer->offset;
    parser_error_t errorCode;
    uint16_t abbreviationLen;
    uint16_t nameLen;
    uint16_t descriptionLen;
    const char *abbreviation = readString(buffer, &abbreviationLen);
    bool isDefault = readBool(buffer);
    const char *name = readString(buffer, &nameLen);
    const char *description = readString(buffer, &descriptionLen);
    uint16_t layerCount = readCompactLength(buffer);

    (void)name;
    (void)description;
    if (!abbreviationLen || abbreviationLen > 3) {
        return ParserError_InvalidAbbreviationLen;
    }
    if (layerCount != LAYER_COUNT) {
        return ParserError_InvalidLayerCount;
    }
    if (!ParserRunDry) {
        AllKeymaps[keymapIdx].abbreviation = abbreviation;
        AllKeymaps[keymapIdx].abbreviationLen = abbreviationLen;
        AllKeymaps[keymapIdx].offset = offset;
        if (isDefault) {
            DefaultKeymapIndex = keymapIdx;
        }
    }
    tempKeymapCount = keymapCount;
    tempMacroCount = macroCount;
    for (uint8_t layerIdx = 0; layerIdx < layerCount; layerIdx++) {
        errorCode = parseLayer(buffer, layerIdx);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

