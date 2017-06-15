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

static void parseNoneAction(key_action_t *action, serialized_buffer_t *buffer) {
    action->type = KeyActionType_None;
}

static void parseKeyStrokeAction(key_action_t *action, uint8_t actionType, serialized_buffer_t *buffer) {
    uint8_t flags = actionType - 1;

    action->type = KeyActionType_Keystroke;

    uint8_t keystrokeType = (SERIALIZED_KEYSTROKE_TYPE_MASK_KEYSTROKE_TYPE & flags) >> SERIALIZED_KEYSTROKE_TYPE_OFFSET_KEYSTROKE_TYPE;
    switch (keystrokeType) {
        case SerializedKeystrokeType_Basic:
            action->keystroke.keystrokeType = KeystrokeType_Basic;
            break;
        case SerializedKeystrokeType_ShortMedia:
        case SerializedKeystrokeType_LongMedia:
            action->keystroke.keystrokeType = KeystrokeType_Media;
            break;
        case SerializedKeystrokeType_System:
            action->keystroke.keystrokeType = KeystrokeType_System;
            break;
    }
    if (flags & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_SCANCODE) {
        action->keystroke.scancode = keystrokeType == SerializedKeystrokeType_LongMedia ? readUInt16(buffer) : readUInt8(buffer);
    }
    if (flags & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_MODIFIERS) {
        action->keystroke.modifiers = readUInt8(buffer);
    }
    if (flags & SERIALIZED_KEYSTROKE_TYPE_MASK_HAS_LONGPRESS) {
        action->keystroke.longPressAction = readUInt8(buffer);
    }
}

static void parseSwitchLayerAction(key_action_t *action, serialized_buffer_t *buffer) {
    uint8_t layer = readUInt8(buffer) + 1;
    bool isToggle = readBool(buffer);

    action->type = KeyActionType_SwitchLayer;
    action->switchLayer.layer = layer;
    action->switchLayer.isToggle = isToggle;
}

static void parseSwitchKeymapAction(key_action_t *action, serialized_buffer_t *buffer) {
//    uint16_t len;
//    const char *keymap = readString(buffer, &len);

    action->type = KeyActionType_SwitchKeymap;

    // TODO: Implement this
}

static void parseMouseAction(key_action_t *action, serialized_buffer_t *buffer) {
    uint8_t mouseAction = readUInt8(buffer);

    action->type = KeyActionType_Mouse;
    switch (mouseAction) {
    case 0: // leftClick
        action->mouse.buttonActions |= MouseButton_Left;
        break;
    case 1: // middleClick
        action->mouse.buttonActions |= MouseButton_Middle;
        break;
    case 2: // rightClick
        action->mouse.buttonActions |= MouseButton_Right;
        break;
    case 3: // moveUp
        action->mouse.moveActions |= MouseMove_Up;
        break;
    case 4: // moveDown
        action->mouse.moveActions |= MouseMove_Down;
        break;
    case 5: // moveLeft
        action->mouse.moveActions |= MouseMove_Left;
        break;
    case 6: // moveRight
        action->mouse.moveActions |= MouseMove_Right;
        break;
    case 7: // scrollUp
        action->mouse.scrollActions |= MouseScroll_Up;
        break;
    case 8: // scrollDown
        action->mouse.scrollActions |= MouseScroll_Down;
        break;
    case 9: // scrollLeft
        action->mouse.scrollActions |= MouseScroll_Left;
        break;
    case 10: // scrollRight
        action->mouse.scrollActions |= MouseScroll_Right;
        break;
    case 11: // accelerate
        action->mouse.moveActions |= MouseMove_Accelerate;
        break;
    case 12: // decelerate
        action->mouse.moveActions |= MouseMove_Decelerate;
        break;
    }
}

static void parseKeyAction(key_action_t *action, serialized_buffer_t *buffer) {
    uint8_t actionType = readUInt8(buffer);

    switch (actionType) {
    case SerializedKeyActionType_None:
        return parseNoneAction(action, buffer);
    case SerializedKeyActionType_KeyStroke ... SerializedKeyActionType_LastKeyStroke:
        return parseKeyStrokeAction(action, actionType, buffer);
    case SerializedKeyActionType_SwitchLayer:
        return parseSwitchLayerAction(action, buffer);
    case SerializedKeyActionType_SwitchKeymap:
        return parseSwitchKeymapAction(action, buffer);
    case SerializedKeyActionType_Mouse:
        return parseMouseAction(action, buffer);
    default:
        // TODO: Handle the case where the actionType is unknown
        break;
    }
}

static void parseKeyActions(uint8_t targetLayer, serialized_buffer_t *buffer, uint8_t moduleID, uint8_t pointerRole) {
    uint8_t actionCount = readCompactLength(buffer);

    for (uint8_t actionIdx = 0; actionIdx < actionCount; actionIdx++) {
        key_action_t *action = &(CurrentKeymap[targetLayer][moduleID][actionIdx]);
        parseKeyAction(action, buffer);
    }
}

static void parseModule(serialized_buffer_t *buffer, uint8_t targetLayer) {
    uint8_t moduleID = readUInt8(buffer);
    uint8_t pointerRole = readUInt8(buffer);
    parseKeyActions(targetLayer, buffer, moduleID, pointerRole);
}

static void clearModule(uint8_t targetLayer, uint8_t moduleID) {
    memset(&CurrentKeymap[targetLayer][moduleID], 0, MAX_KEY_COUNT_PER_MODULE * sizeof(key_action_t));
}

void ParseLayer(const uint8_t *data, uint8_t targetLayer) {
    serialized_buffer_t buffer;

    buffer.buffer = data;
    buffer.offset = 0;

    uint8_t moduleCount = readCompactLength(&buffer);

    for (uint8_t modIdx = 0; modIdx < moduleCount; modIdx++) {
        clearModule(targetLayer, modIdx);
        parseModule(&buffer, targetLayer);
    }
}
