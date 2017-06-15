#include "config/parse_keymap.h"
#include "key_action.h"
#include "current_keymap.h"

#define longCompactLengthPrefix 0xff

static uint8_t readUInt8(serialized_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++];
}

static uint16_t readUInt16(serialized_buffer_t *buffer) {
    uint8_t firstByte = buffer->buffer[buffer->offset++];
    return firstByte + (buffer->buffer[buffer->offset++] << 8);
}

static bool readBool(serialized_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++] == 1;
}

static uint16_t readCompactLength(serialized_buffer_t *buffer) {
    uint16_t length = readUInt8(buffer);
    if (length == longCompactLengthPrefix) {
        length = readUInt16(buffer);
    }
    return length;
}

/*
static const char *readString(serialized_buffer_t *buffer, uint16_t *len) {
    const char *str = (const char *)&(buffer->buffer[buffer->offset]);

    *len = readCompactLength(buffer);
    buffer->offset += *len;

    return str;
}
*/

static void parseNoneAction(key_action_t *keyAction, serialized_buffer_t *buffer) {
    keyAction->type = KeyActionType_None;
}

static void parseKeyStrokeAction(key_action_t *keyAction, uint8_t actionType, serialized_buffer_t *buffer) {
    uint8_t flags = actionType - 1;

    keyAction->type = KeyActionType_Keystroke;

    uint8_t keystrokeType = (SERIALIZED_KEYSTROKE_TYPE_MASK_KEYSTROKE_TYPE & flags) >> SERIALIZED_KEYSTROKE_TYPE_OFFSET_KEYSTROKE_TYPE;
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
    }
    if (flags & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_SCANCODE) {
        keyAction->keystroke.scancode = keystrokeType == SerializedKeystrokeType_LongMedia ? readUInt16(buffer) : readUInt8(buffer);
    }
    if (flags & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_MODIFIERS) {
        keyAction->keystroke.modifiers = readUInt8(buffer);
    }
    if (flags & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_LONGPRESS) {
        keyAction->keystroke.longPressAction = readUInt8(buffer);
    }
}

static void parseSwitchLayerAction(key_action_t *KeyAction, serialized_buffer_t *buffer) {
    uint8_t layer = readUInt8(buffer) + 1;
    bool isToggle = readBool(buffer);

    KeyAction->type = KeyActionType_SwitchLayer;
    KeyAction->switchLayer.layer = layer;
    KeyAction->switchLayer.isToggle = isToggle;
}

static void parseSwitchKeymapAction(key_action_t *keyAction, serialized_buffer_t *buffer) {
//    uint16_t len;
//    const char *keymap = readString(buffer, &len);

    keyAction->type = KeyActionType_SwitchKeymap;

    // TODO: Implement this
}

static void parseMouseAction(key_action_t *keyAction, serialized_buffer_t *buffer) {
    uint8_t mouseAction = readUInt8(buffer);

    keyAction->type = KeyActionType_Mouse;
    switch (mouseAction) {
    case 0: // leftClick
        keyAction->mouse.buttonActions |= MouseButton_Left;
        break;
    case 1: // middleClick
        keyAction->mouse.buttonActions |= MouseButton_Middle;
        break;
    case 2: // rightClick
        keyAction->mouse.buttonActions |= MouseButton_Right;
        break;
    case 3: // moveUp
        keyAction->mouse.moveActions |= MouseMove_Up;
        break;
    case 4: // moveDown
        keyAction->mouse.moveActions |= MouseMove_Down;
        break;
    case 5: // moveLeft
        keyAction->mouse.moveActions |= MouseMove_Left;
        break;
    case 6: // moveRight
        keyAction->mouse.moveActions |= MouseMove_Right;
        break;
    case 7: // scrollUp
        keyAction->mouse.scrollActions |= MouseScroll_Up;
        break;
    case 8: // scrollDown
        keyAction->mouse.scrollActions |= MouseScroll_Down;
        break;
    case 9: // scrollLeft
        keyAction->mouse.scrollActions |= MouseScroll_Left;
        break;
    case 10: // scrollRight
        keyAction->mouse.scrollActions |= MouseScroll_Right;
        break;
    case 11: // accelerate
        keyAction->mouse.moveActions |= MouseMove_Accelerate;
        break;
    case 12: // decelerate
        keyAction->mouse.moveActions |= MouseMove_Decelerate;
        break;
    }
}

static void parseKeyAction(key_action_t *keyAction, serialized_buffer_t *buffer) {
    uint8_t actionType = readUInt8(buffer);

    switch (actionType) {
    case SerializedKeyActionType_None:
        return parseNoneAction(keyAction, buffer);
    case SerializedKeyActionType_KeyStroke ... SerializedKeyActionType_LastKeyStroke:
        return parseKeyStrokeAction(keyAction, actionType, buffer);
    case SerializedKeyActionType_SwitchLayer:
        return parseSwitchLayerAction(keyAction, buffer);
    case SerializedKeyActionType_SwitchKeymap:
        return parseSwitchKeymapAction(keyAction, buffer);
    case SerializedKeyActionType_Mouse:
        return parseMouseAction(keyAction, buffer);
    default:
        // TODO: Handle the case where the actionType is unknown.
        break;
    }
}

static void parseKeyActions(uint8_t targetLayer, serialized_buffer_t *buffer, uint8_t moduleId, uint8_t pointerRole) {
    uint8_t actionCount = readCompactLength(buffer);

    for (uint8_t actionIdx = 0; actionIdx < actionCount; actionIdx++) {
        key_action_t *keyAction = &(CurrentKeymap[targetLayer][moduleId][actionIdx]);
        parseKeyAction(keyAction, buffer);
    }
}

static void parseModule(serialized_buffer_t *buffer, uint8_t layer) {
    uint8_t moduleId = readUInt8(buffer);
    uint8_t pointerRole = readUInt8(buffer);
    parseKeyActions(layer, buffer, moduleId, pointerRole);
}

static void clearModule(uint8_t layer, uint8_t moduleId) {
    memset(&CurrentKeymap[layer][moduleId], 0, MAX_KEY_COUNT_PER_MODULE * sizeof(key_action_t));
}

void ParseLayer(const uint8_t *data, uint8_t layer) {
    serialized_buffer_t buffer;
    buffer.buffer = data;
    buffer.offset = 0;

    uint8_t moduleCount = readCompactLength(&buffer);

    for (uint8_t moduleIdx = 0; moduleIdx < moduleCount; moduleIdx++) {
        clearModule(layer, moduleIdx);
        parseModule(&buffer, layer);
    }
}
