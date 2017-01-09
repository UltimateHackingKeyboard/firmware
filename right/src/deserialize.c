#include "deserialize.h"
#include "action.h"
#include "current_keymap.h"

#define longCompactLengthPrefix 0xff

#define HAS_SCANCODE (1 << 0)
#define HAS_MODS (1 << 1)
#define HAS_LONGPRESS (1 << 2)

typedef struct {
    const uint8_t *buffer;
    uint16_t offset;
} serialized_buffer_t;

enum {
    NoneAction = 0,
    KeyStrokeAction = 1,
    LastKeyStrokeAction = 7,
    SwitchLayerAction,
    SwitchKeymapAction,
    MouseAction,
    PlayMacroAction
};

// ----------------

static uint8_t readUInt8(serialized_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++];
}

static bool readBool(serialized_buffer_t *buffer) {
    return buffer->buffer[buffer->offset++] == 1;
}

static uint16_t readCompactLength(serialized_buffer_t *buffer) {
    uint16_t length = readUInt8(buffer);
    if (length == longCompactLengthPrefix) {
        length += readUInt8(buffer) << 8;
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

static uint8_t findIndex(uint8_t moduleID, uint8_t idx) {
    switch (moduleID) {
    case 0:
        switch (idx) {
        case 0 ... 6:
            return idx;
        case 7:
            return 14;
        case 8 ... 14:
            return idx - 1;
        case 15:
            return 21;
        case 16 ... 21:
            return idx - 1;
        case 22 ... 27:
            return idx;
        case 28:
            return idx + 1;
        case 29 ... 32:
            return idx + 2;
        case 33:
            return 30;
        }
        break;

    case 1:
        switch (idx) {
        case 0 ... 11:
            return idx;
        case 12 ... 17:
            return idx + 1;
        case 18 ... 19:
            return idx + 2;
        case 20 ... 28:
            return idx + 3;
        case 29:
            return 33;
        case 30:
            return 32;
        }
        break;
    }
    return idx;
}

static void processNoneAction(key_action_t *action, serialized_buffer_t *buffer) {
    action->type = KEY_ACTION_NONE;
}

static void processKeyStrokeAction(key_action_t *action, uint8_t actionType, serialized_buffer_t *buffer) {
    uint8_t flags = actionType - 1;

    action->type = KEY_ACTION_KEYSTROKE;

    if (flags & HAS_SCANCODE) {
        action->keystroke.key = readUInt8(buffer);
    }
    if (flags & HAS_MODS) {
        action->keystroke.mods = readUInt8(buffer);
    }
    if (flags & HAS_LONGPRESS) {
        action->keystroke.longPress = readUInt8(buffer);
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
        uint8_t pos = findIndex(moduleID, actionIdx);
        key_action_t *action = &(CurrentKeymap[targetLayer][moduleID][pos]);

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
