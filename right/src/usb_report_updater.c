#include "main.h"
#include "action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interface_mouse.h"
#include "current_keymap.h"

static uint8_t activeLayer = LAYER_ID_BASE;
static uint8_t mouseWheelDivisorCounter = 0;
static uint8_t mouseSpeedAccelDivisorCounter = 0;
static uint8_t mouseSpeed = 3;
static bool wasPreviousMouseActionWheelAction = false;

void ProcessMouseAction(key_action_t action)
{
    bool isWheelAction = action.mouse.scrollActions && !action.mouse.moveActions && !action.mouse.buttonActions;

    if (isWheelAction && wasPreviousMouseActionWheelAction) {
        mouseWheelDivisorCounter++;
    }

    if (action.mouse.scrollActions) {
        if (mouseWheelDivisorCounter == MOUSE_WHEEL_DIVISOR) {
            mouseWheelDivisorCounter = 0;
            if (action.mouse.scrollActions & MOUSE_SCROLL_UP) {
                UsbMouseReport.wheelX = 1;
            }
            if (action.mouse.scrollActions & MOUSE_SCROLL_DOWN) {
                UsbMouseReport.wheelX = -1;
            }
        }
    }

    if (action.mouse.moveActions & MOUSE_ACCELERATE || action.mouse.moveActions & MOUSE_DECELERATE) {
        mouseSpeedAccelDivisorCounter++;

        if (mouseSpeedAccelDivisorCounter == MOUSE_SPEED_ACCEL_DIVISOR) {
            mouseSpeedAccelDivisorCounter = 0;

            if (action.mouse.moveActions & MOUSE_ACCELERATE) {
                if (mouseSpeed < MOUSE_MAX_SPEED) {
                    mouseSpeed++;
                }
            }
            if (action.mouse.moveActions & MOUSE_DECELERATE) {
                if (mouseSpeed > 1) {
                    mouseSpeed--;
                }
            }
        }
    } else if (action.mouse.moveActions) {
        if (action.mouse.moveActions & MOUSE_MOVE_LEFT) {
            UsbMouseReport.x = -mouseSpeed;
        }
        if (action.mouse.moveActions & MOUSE_MOVE_RIGHT) {
            UsbMouseReport.x = mouseSpeed;
        }
        if (action.mouse.moveActions & MOUSE_MOVE_UP) {
            UsbMouseReport.y = -mouseSpeed;
        }
        if (action.mouse.moveActions & MOUSE_MOVE_DOWN) {
            UsbMouseReport.y = mouseSpeed;
        }
    }

    UsbMouseReport.buttons |= action.mouse.buttonActions;

    wasPreviousMouseActionWheelAction = isWheelAction;
}

void UpdateActiveUsbReports() {

    bzero(&UsbMouseReport, sizeof(usb_mouse_report_t));

    uint8_t scancodeIdx = 0;

    activeLayer = LAYER_ID_BASE;
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            if (CurrentKeyStates[slotId][keyId]) {
                key_action_t action = CurrentKeymap[LAYER_ID_BASE][slotId][keyId];
                if (action.type == KEY_ACTION_SWITCH_LAYER) {
                    activeLayer = action.switchLayer.layer;
                }
            }
        }
    }

    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {

            if (!CurrentKeyStates[slotId][keyId]) {
                continue;
            }

            key_action_t action = CurrentKeymap[activeLayer][slotId][keyId];
            switch (action.type) {
                case KEY_ACTION_KEYSTROKE:
                    ActiveUsbKeyboardReport->scancodes[scancodeIdx++] = action.keystroke.key;
                    ActiveUsbKeyboardReport->modifiers |= action.keystroke.mods;
                    break;
                case KEY_ACTION_MOUSE:
                    ProcessMouseAction(action);
                    break;
            }
        }
    }
}
