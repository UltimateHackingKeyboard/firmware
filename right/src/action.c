#include "main.h"
#include "action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interface_mouse.h"
#include "current_keymap.h"

static uint8_t keyMasks[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

static uint8_t ActiveLayer = LAYER_ID_BASE;

static key_action_t getKeycode(uint8_t slotId, uint8_t keyId)
{
    if (keyId < MAX_KEY_COUNT_PER_MODULE) {
        if (keyMasks[slotId][keyId]!=0 && keyMasks[slotId][keyId]!=ActiveLayer) {
            // Mask out key presses after releasing modifier keys
            return (key_action_t){.type = KEY_ACTION_NONE};
        }

        key_action_t key = CurrentKeymap[ActiveLayer][slotId][keyId];
        keyMasks[slotId][keyId] = ActiveLayer;

        return key;
    } else {
        return (key_action_t){.type = KEY_ACTION_NONE};
    }
}

static void clearKeymask(const uint8_t *keyStates)
{
    for (uint8_t i=0; i < MAX_KEY_COUNT_PER_MODULE; i++) {
        if (keyStates[i]==0) {
            keyMasks[SLOT_ID_LEFT_KEYBOARD_HALF][i] = 0;
        }
    }
}

static bool pressKey(key_action_t key, int scancodeIdx, usb_keyboard_report_t *report)
{
    if (key.type != KEY_ACTION_KEYSTROKE) {
        return false;
    }

    if (!key.keystroke.key) {
        return false;
    }

    for (uint8_t i = 0; i < 8; i++) {
        if (key.keystroke.mods & (1 << i) || key.keystroke.key == HID_KEYBOARD_SC_LEFT_CONTROL + i) {
            report->modifiers |= (1 << i);
        }
    }

    report->scancodes[scancodeIdx] = key.keystroke.key;
    return true;
}

static bool hasKeyPressed(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId)
{
    return (!prevKeyStates[keyId]) && currKeyStates[keyId];
}

static bool isKeyPressed(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId)
{
    return currKeyStates[keyId];
}

static bool hasKeyReleased(const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId)
{
    return (!currKeyStates[keyId]) && prevKeyStates[keyId];
}

static bool handleKey(key_action_t key, int scancodeIdx, usb_keyboard_report_t *report, const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId) {
    switch (key.type) {
    case KEY_ACTION_KEYSTROKE:
        if (isKeyPressed(prevKeyStates, currKeyStates, keyId)) {
            return pressKey(key, scancodeIdx, report);
        }
        break;
    case KEY_ACTION_SWITCH_LAYER:
        if (hasKeyPressed(prevKeyStates, currKeyStates, keyId)) {
            ActiveLayer = key.switchLayer.layer;
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

void HandleMouseKey(usb_mouse_report_t *report, key_action_t key, const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId)
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
            if (key.mouse.scrollActions & MOUSE_SCROLL_UP) {
                    report->wheelX = 1;
            }
            if (key.mouse.scrollActions & MOUSE_SCROLL_DOWN) {
                report->wheelX = -1;
            }
        }
    }

    if (key.mouse.moveActions & MOUSE_ACCELERATE || key.mouse.moveActions & MOUSE_DECELERATE) {
        mouseSpeedAccelDivisorCounter++;

        if (mouseSpeedAccelDivisorCounter == MOUSE_SPEED_ACCEL_DIVISOR) {
            mouseSpeedAccelDivisorCounter = 0;

            if (key.mouse.moveActions & MOUSE_ACCELERATE) {
                if (mouseSpeed < MOUSE_MAX_SPEED) {
                    mouseSpeed++;
                }
            }
            if (key.mouse.moveActions & MOUSE_DECELERATE) {
                if (mouseSpeed > 1) {
                    mouseSpeed--;
                }
            }
        }
    } else if (key.mouse.moveActions) {
        if (key.mouse.moveActions & MOUSE_MOVE_LEFT) {
            report->x = -mouseSpeed;
        }
        if (key.mouse.moveActions & MOUSE_MOVE_RIGHT) {
            report->x = mouseSpeed;
        }
        if (key.mouse.moveActions & MOUSE_MOVE_UP) {
            report->y = -mouseSpeed;
        }
        if (key.mouse.moveActions & MOUSE_MOVE_DOWN) {
            report->y = mouseSpeed;
        }
    }

    report->buttons |= key.mouse.buttonActions;

    wasPreviousMouseActionWheelAction = isWheelAction;
}

void HandleKeyboardEvents(usb_keyboard_report_t *keyboardReport, usb_mouse_report_t *mouseReport) {
    int scancodeIdx = 0;

    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        clearKeymask(CurrentKeyStates[slotId]);
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            if (scancodeIdx >= USB_KEYBOARD_MAX_KEYS) {
                break;
            }

            key_action_t code = getKeycode(slotId, keyId);

            if (code.type == KEY_ACTION_MOUSE) {
                HandleMouseKey(mouseReport, code, PreviousKeyStates[slotId], CurrentKeyStates[slotId], keyId);
            } else {
                if (handleKey(code, scancodeIdx, keyboardReport, PreviousKeyStates[slotId], CurrentKeyStates[slotId], keyId)) {
                    scancodeIdx++;
                }
            }
        }

        memcpy(PreviousKeyStates[slotId], CurrentKeyStates[slotId], MAX_KEY_COUNT_PER_MODULE);
    }
}
