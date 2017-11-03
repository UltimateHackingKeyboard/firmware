#include "main.h"
#include "key_action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "keymaps.h"
#include "peripherals/test_led.h"
#include "slave_drivers/is31fl3731_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "led_pwm.h"
#include "macros.h"
#include "key_states.h"
#include "right_key_matrix.h"

static uint8_t mouseWheelDivisorCounter = 0;
static uint8_t mouseSpeedAccelDivisorCounter = 0;
static uint8_t mouseSpeed = 3;
static bool wasPreviousMouseActionWheelAction = false;

void processMouseAction(key_action_t action)
{
    bool isWheelAction = action.mouse.scrollActions && !action.mouse.moveActions && !action.mouse.buttonActions;

    if (isWheelAction && wasPreviousMouseActionWheelAction) {
        mouseWheelDivisorCounter++;
    }

    if (action.mouse.scrollActions) {
        if (mouseWheelDivisorCounter == MOUSE_WHEEL_DIVISOR) {
            mouseWheelDivisorCounter = 0;
            if (action.mouse.scrollActions & MouseScroll_Up) {
                ActiveUsbMouseReport->wheelX = 1;
            }
            if (action.mouse.scrollActions & MouseScroll_Down) {
                ActiveUsbMouseReport->wheelX = -1;
            }
        }
    }

    if (action.mouse.moveActions & MouseMove_Accelerate || action.mouse.moveActions & MouseMove_Decelerate) {
        mouseSpeedAccelDivisorCounter++;

        if (mouseSpeedAccelDivisorCounter == MOUSE_SPEED_ACCEL_DIVISOR) {
            mouseSpeedAccelDivisorCounter = 0;

            if (action.mouse.moveActions & MouseMove_Accelerate) {
                if (mouseSpeed < MOUSE_MAX_SPEED) {
                    mouseSpeed++;
                }
            }
            if (action.mouse.moveActions & MouseMove_Decelerate) {
                if (mouseSpeed > 1) {
                    mouseSpeed--;
                }
            }
        }
    } else if (action.mouse.moveActions) {
        if (action.mouse.moveActions & MouseMove_Left) {
            ActiveUsbMouseReport->x = -mouseSpeed;
        }
        if (action.mouse.moveActions & MouseMove_Right) {
            ActiveUsbMouseReport->x = mouseSpeed;
        }
        if (action.mouse.moveActions & MouseMove_Up) {
            ActiveUsbMouseReport->y = -mouseSpeed;
        }
        if (action.mouse.moveActions & MouseMove_Down) {
            ActiveUsbMouseReport->y = mouseSpeed;
        }
    }

    ActiveUsbMouseReport->buttons |= action.mouse.buttonActions;

    wasPreviousMouseActionWheelAction = isWheelAction;
}

static bool pressedLayers[LAYER_COUNT];

void updatePressedLayers(void)
{
    memset(pressedLayers, false, LAYER_COUNT);

    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            if (KeyStates[slotId][keyId].current) {
                key_action_t action = CurrentKeymap[LayerId_Base][slotId][keyId];
                if (action.type == KeyActionType_SwitchLayer) {
                    pressedLayers[action.switchLayer.layer] = true;
                }
            }
        }
    }
}

static uint8_t PreviousLayer = LayerId_Base;

layer_id_t getActiveLayer()
{
    updatePressedLayers();

    layer_id_t activeLayer = LayerId_Base;

    for (layer_id_t layerId=LayerId_Mod; layerId<=LayerId_Mouse; layerId++) {
        if (pressedLayers[layerId]) {
            activeLayer = layerId;
            break;
        }
    }

    activeLayer = activeLayer != LayerId_Base && pressedLayers[PreviousLayer] ? PreviousLayer : activeLayer;
    PreviousLayer = activeLayer;

    return activeLayer;
}

void UpdateActiveUsbReports(void)
{
    uint8_t basicScancodeIndex = 0;
    uint8_t mediaScancodeIndex = 0;
    uint8_t systemScancodeIndex = 0;

    layer_id_t activeLayer = getActiveLayer();
    LedDisplay_SetLayer(activeLayer);
    static uint8_t previousModifiers = 0;

    for (uint8_t keyId=0; keyId < RIGHT_KEY_MATRIX_KEY_COUNT; keyId++) {
        KeyStates[SlotId_RightKeyboardHalf][keyId].current = RightKeyMatrix.keyStates[keyId];
    }

    if (MacroPlaying) {
        Macros_ContinueMacro();
        memcpy(&ActiveUsbMouseReport, &MacroMouseReport, sizeof MacroMouseReport);
        memcpy(&ActiveUsbBasicKeyboardReport, &MacroBasicKeyboardReport, sizeof MacroBasicKeyboardReport);
        memcpy(&ActiveUsbMediaKeyboardReport, &MacroMediaKeyboardReport, sizeof MacroMediaKeyboardReport);
        memcpy(&ActiveUsbSystemKeyboardReport, &MacroSystemKeyboardReport, sizeof MacroSystemKeyboardReport);
        return;
    }


    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t keyState = KeyStates[slotId][keyId];
            if (keyState.current) {
                key_action_t action = CurrentKeymap[activeLayer][slotId][keyId];
                switch (action.type) {
                    case KeyActionType_Keystroke:
                        ActiveUsbBasicKeyboardReport->modifiers |= action.keystroke.modifiers;

                        switch (action.keystroke.keystrokeType) {
                            case KeystrokeType_Basic:
                                if (basicScancodeIndex >= USB_BASIC_KEYBOARD_MAX_KEYS) {
                                    break;
                                }
                                ActiveUsbBasicKeyboardReport->scancodes[basicScancodeIndex++] = action.keystroke.scancode;
                                break;
                            case KeystrokeType_Media:
                                if (mediaScancodeIndex >= USB_MEDIA_KEYBOARD_MAX_KEYS) {
                                    break;
                                }
                                ActiveUsbMediaKeyboardReport->scancodes[mediaScancodeIndex++] = action.keystroke.scancode;
                                break;
                            case KeystrokeType_System:
                                if (systemScancodeIndex >= USB_SYSTEM_KEYBOARD_MAX_KEYS) {
                                    break;
                                }
                                ActiveUsbSystemKeyboardReport->scancodes[systemScancodeIndex++] = action.keystroke.scancode;
                                break;
                        }
                        break;
                    case KeyActionType_Mouse:
                        processMouseAction(action);
                        break;
                    case KeyActionType_SwitchKeymap:
                        Keymaps_Switch(action.switchKeymap.keymapId);
                        break;
                }
            }
            keyState.previous = keyState.current;
        }
    }

    // When a layer switcher key gets pressed along with another key that produces some modifiers
    // and the accomanying key gets released then keep the related modifiers active a long as the
    // layer switcher key stays pressed.  Useful for Alt+Tab keymappings and the like.
    if (activeLayer != LayerId_Base && activeLayer == PreviousLayer && basicScancodeIndex == 0) {
        ActiveUsbBasicKeyboardReport->modifiers |= previousModifiers;
    }

    previousModifiers = ActiveUsbBasicKeyboardReport->modifiers;
}
