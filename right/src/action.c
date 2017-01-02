#include "action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interface_mouse.h"

static uint8_t keyMasks[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

static uint8_t ActiveLayer = LAYER_ID_BASE;
uint8_t prevKeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

static inline __attribute__((always_inline)) uhk_key_t getKeycode(uint8_t slotId, uint8_t keyId)
{
    if (keyId < MAX_KEY_COUNT_PER_MODULE) {
        if (keyMasks[slotId][keyId]!=0 && keyMasks[slotId][keyId]!=ActiveLayer) {
            // Mask out key presses after releasing modifier keys
            return (uhk_key_t){.type = UHK_KEY_NONE};
        }

        uhk_key_t key = CurrentKeymap[ActiveLayer][slotId][keyId];
        keyMasks[slotId][keyId] = ActiveLayer;

        return key;
    } else {
        return (uhk_key_t){.type = UHK_KEY_NONE};
    }
}

static void clearKeymasks(const uint8_t *leftKeyStates, const uint8_t *rightKeyStates){
    int i;
    for (i=0; i < MAX_KEY_COUNT_PER_MODULE; i++){
        if (rightKeyStates[i]==0){
            keyMasks[SLOT_ID_RIGHT_KEYBOARD_HALF][i] = 0;
        }

        if (leftKeyStates[i]==0) {
            keyMasks[SLOT_ID_LEFT_KEYBOARD_HALF][i] = 0;
        }
    }
}

static bool pressKey(uhk_key_t key, int scancodeIdx, usb_keyboard_report_t *report) {
    if (key.type != UHK_KEY_SIMPLE) {
        return false;
    }

    if (!key.simple.key) {
        return false;
    }

    for (uint8_t i = 0; i < 8; i++) {
        if (key.simple.mods & (1 << i) || key.simple.key == HID_KEYBOARD_SC_LEFT_CONTROL + i) {
            report->modifiers |= (1 << i);
        }
    }

    report->scancodes[scancodeIdx] = key.simple.key;
    return true;
}

static bool hasKeyPressed(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    return (!prevKeyStates[keyId]) && currKeyStates[keyId];
}

static bool isKeyPressed(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    return currKeyStates[keyId];
}

static bool hasKeyReleased(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    return (!currKeyStates[keyId]) && prevKeyStates[keyId];
}

static bool handleKey(uhk_key_t key, int scancodeIdx, usb_keyboard_report_t *report, const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    switch (key.type) {
    case UHK_KEY_SIMPLE:
        if (isKeyPressed(prevKeyStates, currKeyStates, keyId)) {
            return pressKey(key, scancodeIdx, report);
        }
        break;
    case UHK_KEY_LAYER:
        if (hasKeyPressed(prevKeyStates, currKeyStates, keyId)) {
            ActiveLayer = key.layer.target;
        }
        if (hasKeyReleased(prevKeyStates, currKeyStates, keyId)) {
            ActiveLayer = LAYER_ID_BASE;
        }
        LedDisplay_SetLayerLed(ActiveLayer);
        return false;
        break;
    default:
        break;
    }
    return false;
}

static uint8_t mouseWheelDivisorCounter = 0;
static uint8_t mouseSpeedAccelDivisorCounter = 0;
static uint8_t mouseSpeed = 3;
static bool wasPreviousMouseActionWheelAction = false;

static void handleMouseKey(usb_mouse_report_t *report, uhk_key_t key, const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId)
{
    if (!isKeyPressed(prevKeyStates, currKeyStates, keyId)) {
        return;
    }

    bool isWheelAction = key.mouse.scrollActions && !key.mouse.moveActions && !key.mouse.buttonActions;

    if (isWheelAction && wasPreviousMouseActionWheelAction) {
        mouseWheelDivisorCounter++;
    }

    if (key.mouse.scrollActions) {
        if (mouseWheelDivisorCounter == MOUSE_WHEEL_DIVISOR) {
            mouseWheelDivisorCounter = 0;
            if (key.mouse.scrollActions & UHK_MOUSE_SCROLL_UP) {
                    report->wheelX = 1;
            }
            if (key.mouse.scrollActions & UHK_MOUSE_SCROLL_DOWN) {
                report->wheelX = -1;
            }
        }
    }

    if (key.mouse.moveActions & UHK_MOUSE_ACCELERATE || key.mouse.moveActions & UHK_MOUSE_DECELERATE) {
        mouseSpeedAccelDivisorCounter++;

        if (mouseSpeedAccelDivisorCounter == MOUSE_SPEED_ACCEL_DIVISOR) {
            mouseSpeedAccelDivisorCounter = 0;

            if (key.mouse.moveActions & UHK_MOUSE_ACCELERATE) {
                if (mouseSpeed < MOUSE_MAX_SPEED) {
                    mouseSpeed++;
                }
            }
            if (key.mouse.moveActions & UHK_MOUSE_DECELERATE) {
                if (mouseSpeed > 1) {
                    mouseSpeed--;
                }
            }
        }
    } else if (key.mouse.moveActions) {
        if (key.mouse.moveActions & UHK_MOUSE_MOVE_LEFT) {
            report->x = -mouseSpeed;
        }
        if (key.mouse.moveActions & UHK_MOUSE_MOVE_RIGHT) {
            report->x = mouseSpeed;
        }
        if (key.mouse.moveActions & UHK_MOUSE_MOVE_UP) {
            report->y = -mouseSpeed;
        }
        if (key.mouse.moveActions & UHK_MOUSE_MOVE_DOWN) {
            report->y = mouseSpeed;
        }
    }

    report->buttons |= key.mouse.buttonActions;

    wasPreviousMouseActionWheelAction = isWheelAction;
}

void HandleKeyboardEvents(usb_keyboard_report_t *keyboardReport, usb_mouse_report_t *mouseReport, const uint8_t *leftKeyStates, const uint8_t *rightKeyStates) {
    int scancodeIdx = 0;

    clearKeymasks(leftKeyStates, rightKeyStates);

    for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
        if (scancodeIdx >= USB_KEYBOARD_MAX_KEYS) {
            break;
        }

        uhk_key_t code = getKeycode(SLOT_ID_RIGHT_KEYBOARD_HALF, keyId);

        if (code.type == UHK_KEY_MOUSE) {
            handleMouseKey(mouseReport, code, prevKeyStates[SLOT_ID_RIGHT_KEYBOARD_HALF], rightKeyStates, keyId);
        } else {
            if (handleKey(code, scancodeIdx, keyboardReport, prevKeyStates[SLOT_ID_RIGHT_KEYBOARD_HALF], rightKeyStates, keyId)) {
                scancodeIdx++;
            }
        }
    }

    for (uint8_t keyId=0; keyId<KEY_STATE_COUNT; keyId++) {
        if (scancodeIdx >= USB_KEYBOARD_MAX_KEYS) {
            break;
        }

        uhk_key_t code = getKeycode(SLOT_ID_LEFT_KEYBOARD_HALF, keyId);

        if (code.type == UHK_KEY_MOUSE) {
            handleMouseKey(mouseReport, code, prevKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], leftKeyStates, keyId);
        } else {
            if (handleKey(code, scancodeIdx, keyboardReport, prevKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], leftKeyStates, keyId)) {
                scancodeIdx++;
            }
        }
    }

    memcpy(prevKeyStates[SLOT_ID_RIGHT_KEYBOARD_HALF], rightKeyStates, KEY_STATE_COUNT);
    memcpy(prevKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], leftKeyStates, KEY_STATE_COUNT);
}
