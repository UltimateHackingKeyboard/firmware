#include "config/parse_keymap.h"
#include "action.h"
#include "current_keymap.h"

#define longCompactLengthPrefix 0xff

#define HAS_SCANCODE   0b00001
#define HAS_MODS       0b00010
#define HAS_LONGPRESS  0b00100
#define KEYSTROKE_TYPE 0b11000
#define KEYSTROKE_TYPE_SHIFT 3

typedef enum {
    KeystrokeType_Basic,
    KeystrokeType_ShortMedia,
    KeystrokeType_LongMedia,
    KeystrokeType_System,
} serialized_keystroke_type_t;

typedef struct {
    const uint8_t *buffer;
    uint16_t offset;
} serialized_buffer_t;

enum {
    NoneAction = 0,
    KeyStrokeAction = 1,
    LastKeyStrokeAction = 31,
    SwitchLayerAction,
    SwitchKeymapAction,
    MouseAction,
    PlayMacroAction
};

// ----------------

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

// ----------------

static void processNoneAction(key_action_t *action, serialized_buffer_t *buffer) {
    action->type = KEY_ACTION_NONE;
}

static void processKeyStrokeAction(key_action_t *action, uint8_t actionType, serialized_buffer_t *buffer) {
    uint8_t flags = actionType - 1;

    action->type = KEY_ACTION_KEYSTROKE;

    uint8_t keystrokeType = (KEYSTROKE_TYPE & flags) >> KEYSTROKE_TYPE_SHIFT;
    switch (keystrokeType) {
        case KeystrokeType_Basic:
            action->keystroke.keystrokeType = KEYSTROKE_BASIC;
            break;
        case KeystrokeType_ShortMedia:
        case KeystrokeType_LongMedia:
            action->keystroke.keystrokeType = KEYSTROKE_MEDIA;
            break;
        case KeystrokeType_System:
            action->keystroke.keystrokeType = KEYSTROKE_SYSTEM;
            break;
    }
    if (flags & HAS_SCANCODE) {
        action->keystroke.scancode = keystrokeType == KeystrokeType_LongMedia ? readUInt16(buffer) : readUInt8(buffer);
    }
    if (flags & HAS_MODS) {
        action->keystroke.modifiers = readUInt8(buffer);
    }
    if (flags & HAS_LONGPRESS) {
        action->keystroke.longPressAction = readUInt8(buffer);
    }
}

static void processSwitchLayerAction(key_action_t *action, serialized_buffer_t *buffer) {
    uint8_t layer = readUInt8(buffer) + 1;
    bool isToggle = readBool(buffer);

    action->type = KEY_ACTION_SWITCH_LAYER;
    action->switchLayer.layer = layer;
    action->switchLayer.isToggle = isToggle;
}

static void processSwitchKeymapAction(key_action_t *action, serialized_buffer_t *buffer) {
//    uint16_t len;
//    const char *keymap = readString(buffer, &len);

    action->type = KEY_ACTION_SWITCH_KEYMAP;

    // TODO: Implement this
}

static void processMouseAction(key_action_t *action, serialized_buffer_t *buffer) {
    uint8_t mouseAction = readUInt8(buffer);

    action->type = KEY_ACTION_MOUSE;
    switch (mouseAction) {
    case 0: // leftClick
        action->mouse.buttonActions |= MOUSE_BUTTON_LEFT;
        break;
    case 1: // middleClick
        action->mouse.buttonActions |= MOUSE_BUTTON_MIDDLE;
        break;
    case 2: // rightClick
        action->mouse.buttonActions |= MOUSE_BUTTON_RIGHT;
        break;
    case 3: // moveUp
        action->mouse.moveActions |= MOUSE_MOVE_UP;
        break;
    case 4: // moveDown
        action->mouse.moveActions |= MOUSE_MOVE_DOWN;
        break;
    case 5: // moveLeft
        action->mouse.moveActions |= MOUSE_MOVE_LEFT;
        break;
    case 6: // moveRight
        action->mouse.moveActions |= MOUSE_MOVE_RIGHT;
        break;
    case 7: // scrollUp
        action->mouse.scrollActions |= MOUSE_SCROLL_UP;
        break;
    case 8: // scrollDown
        action->mouse.scrollActions |= MOUSE_SCROLL_DOWN;
        break;
    case 9: // scrollLeft
        action->mouse.scrollActions |= MOUSE_SCROLL_LEFT;
        break;
    case 10: // scrollRight
        action->mouse.scrollActions |= MOUSE_SCROLL_RIGHT;
        break;
    case 11: // accelerate
        action->mouse.moveActions |= MOUSE_ACCELERATE;
        break;
    case 12: // decelerate
        action->mouse.moveActions |= MOUSE_DECELERATE;
        break;
    }
}

static void processKeyAction(key_action_t *action, serialized_buffer_t *buffer) {
    uint8_t actionType = readUInt8(buffer);

    switch (actionType) {
    case NoneAction:
        return processNoneAction(action, buffer);
    case KeyStrokeAction ... LastKeyStrokeAction:
        return processKeyStrokeAction(action, actionType, buffer);
    case SwitchLayerAction:
        return processSwitchLayerAction(action, buffer);
    case SwitchKeymapAction:
        return processSwitchKeymapAction(action, buffer);
    case MouseAction:
        return processMouseAction(action, buffer);
    default:
        // TODO: Handle the case where the actionType is unknown
        break;
    }
}

static void processKeyActions(uint8_t targetLayer, serialized_buffer_t *buffer, uint8_t moduleID, uint8_t pointerRole) {
    uint8_t actionCount = readCompactLength(buffer);

    for (uint8_t actionIdx = 0; actionIdx < actionCount; actionIdx++) {
        key_action_t *action = &(CurrentKeymap[targetLayer][moduleID][actionIdx]);
        processKeyAction(action, buffer);
    }
}

static void processModule(serialized_buffer_t *buffer, uint8_t targetLayer) {
    uint8_t moduleID, pointerRole;

    moduleID = readUInt8(buffer);
    pointerRole = readUInt8(buffer);

    processKeyActions(targetLayer, buffer, moduleID, pointerRole);
}

static void clearModule(uint8_t targetLayer, uint8_t moduleID) {
    memset(&CurrentKeymap[targetLayer][moduleID], 0, MAX_KEY_COUNT_PER_MODULE * sizeof(key_action_t));
}

// ----------

void deserialize_Layer (const uint8_t *data, uint8_t targetLayer) {
    serialized_buffer_t buffer;

    buffer.buffer = data;
    buffer.offset = 0;

    uint8_t moduleCount = readCompactLength(&buffer);

    for (uint8_t modIdx = 0; modIdx < moduleCount; modIdx++) {
        clearModule(targetLayer, modIdx);
        processModule(&buffer, targetLayer);
    }
}
