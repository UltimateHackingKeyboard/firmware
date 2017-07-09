#include "config/parse_keymap.h"
#include "key_action.h"
#include "current_keymap.h"

static bool isDryRun;

static parser_error_t parseNoneAction(key_action_t *keyAction, serialized_buffer_t *buffer) {
    keyAction->type = KeyActionType_None;
    return ParserError_Success;
}

static parser_error_t parseKeyStrokeAction(key_action_t *keyAction, uint8_t keyStrokeAction, serialized_buffer_t *buffer) {
    keyAction->type = KeyActionType_Keystroke;

    uint8_t keystrokeType = (SERIALIZED_KEYSTROKE_TYPE_MASK_KEYSTROKE_TYPE & keyStrokeAction) >> SERIALIZED_KEYSTROKE_TYPE_OFFSET_KEYSTROKE_TYPE;
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
    if (keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_SCANCODE) {
        keyAction->keystroke.scancode = keystrokeType == SerializedKeystrokeType_LongMedia ? readUInt16(buffer) : readUInt8(buffer);
    }
    if (keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_MODIFIERS) {
        keyAction->keystroke.modifiers = readUInt8(buffer);
    }
    if (keyStrokeAction & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_LONGPRESS) {
        keyAction->keystroke.longPressAction = readUInt8(buffer);
    }
    return ParserError_Success;
}

static parser_error_t parseSwitchLayerAction(key_action_t *KeyAction, serialized_buffer_t *buffer) {
    uint8_t layer = readUInt8(buffer) + 1;
    bool isToggle = readBool(buffer);

    KeyAction->type = KeyActionType_SwitchLayer;
    KeyAction->switchLayer.layer = layer;
    KeyAction->switchLayer.isToggle = isToggle;
    return ParserError_Success;
}

static parser_error_t parseSwitchKeymapAction(key_action_t *keyAction, serialized_buffer_t *buffer) {
    uint16_t keymapAbbreviationLen;
    const char *keymapAbbreviation = readString(buffer, &keymapAbbreviationLen);

    (void)keymapAbbreviation;
    keyAction->type = KeyActionType_SwitchKeymap;
    // TODO: Implement this
    return ParserError_Success;
}

static parser_error_t parseMouseAction(key_action_t *keyAction, serialized_buffer_t *buffer) {
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

static parser_error_t parseKeyAction(key_action_t *keyAction, serialized_buffer_t *buffer) {
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
    default:
        return ParserError_InvalidSerializedKeyActionType;
    }
    return ParserError_Success;
}

static parser_error_t parseKeyActions(uint8_t targetLayer, serialized_buffer_t *buffer, uint8_t moduleId, uint8_t pointerRole) {
    parser_error_t errorCode;
    uint16_t actionCount = readCompactLength(buffer);
    key_action_t dummyKeyAction;

    if (actionCount > MAX_KEY_COUNT_PER_MODULE) {
        return ParserError_InvalidActionCount;
    }
    for (uint16_t actionIdx = 0; actionIdx < actionCount; actionIdx++) {
        errorCode = parseKeyAction(isDryRun ? &dummyKeyAction : &CurrentKeymap[targetLayer][moduleId][actionIdx], buffer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

static parser_error_t parseModule(serialized_buffer_t *buffer, uint8_t layer) {
    uint8_t moduleId = readUInt8(buffer);
    uint8_t pointerRole = readUInt8(buffer);
    return parseKeyActions(layer, buffer, moduleId, pointerRole);
}

static parser_error_t parseLayer(serialized_buffer_t *buffer, uint8_t layer) {
    parser_error_t errorCode;
    uint16_t moduleCount = readCompactLength(buffer);

    if (moduleCount > SLOT_COUNT) {
        return ParserError_InvalidModuleCount;
    }
    for (uint16_t moduleIdx = 0; moduleIdx < moduleCount; moduleIdx++) {
        errorCode = parseModule(buffer, layer);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

parser_error_t ParseKeymap(serialized_buffer_t *buffer) {;
    parser_error_t errorCode;
    uint16_t abbreviationLen;
    uint16_t nameLen;
    uint16_t descriptionLen;
    const char *abbreviation = readString(buffer, &abbreviationLen);
    bool isDefault = readBool(buffer);
    const char *name = readString(buffer, &nameLen);
    const char *description = readString(buffer, &descriptionLen);
    uint16_t layerCount = readCompactLength(buffer);

    (void)abbreviation;
    (void)name;
    (void)description;
    if (layerCount != LAYER_COUNT) {
        return ParserError_InvalidLayerCount;
    }
    isDryRun = !isDefault;
    for (uint16_t layerIdx = 0; layerIdx < layerCount; layerIdx++) {
        errorCode = parseLayer(buffer, layerIdx);
        if (errorCode != ParserError_Success) {
            return errorCode;
        }
    }
    return ParserError_Success;
}

