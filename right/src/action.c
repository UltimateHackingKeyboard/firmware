#include "main.h"
#include "action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interface_mouse.h"
#include "current_keymap.h"

static uint8_t ActiveLayer = LAYER_ID_BASE;

static bool isKeyPressed(const uint8_t *currKeyStates, uint8_t keyId)
{
    return currKeyStates[keyId];
}

static uint8_t mouseWheelDivisorCounter = 0;
static uint8_t mouseSpeedAccelDivisorCounter = 0;
static uint8_t mouseSpeed = 3;
static bool wasPreviousMouseActionWheelAction = false;

void HandleMouseKey(usb_mouse_report_t *report, key_action_t key, const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId)
{
    if (!isKeyPressed(currKeyStates, keyId)) {
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

void HandleKeyboardEvents() {
    int scancodeIdx = 0;

    ActiveLayer = LAYER_ID_BASE;
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            if (CurrentKeyStates[slotId][keyId]) {
                key_action_t action = CurrentKeymap[LAYER_ID_BASE][slotId][keyId];
                if (action.type == KEY_ACTION_SWITCH_LAYER) {
                    ActiveLayer = action.switchLayer.layer;
                }
            }
        }
    }

    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {

            if (!CurrentKeyStates[slotId][keyId]) {
                continue;
            }

            key_action_t action = CurrentKeymap[ActiveLayer][slotId][keyId];
            switch (action.type) {
                case KEY_ACTION_KEYSTROKE:
                    ActiveUsbKeyboardReport->scancodes[scancodeIdx++] = action.keystroke.key;
                    ActiveUsbKeyboardReport->modifiers |= action.keystroke.mods;
                    break;
                case KEY_ACTION_MOUSE:
                    HandleMouseKey(&UsbMouseReport, action, PreviousKeyStates[slotId], CurrentKeyStates[slotId], keyId);
                    break;
            }
        }

        memcpy(PreviousKeyStates[slotId], CurrentKeyStates[slotId], MAX_KEY_COUNT_PER_MODULE);
    }
}
