#include "main.h"
#include "action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "current_keymap.h"
#include "test_states.h"
#include "peripherals/test_led.h"
#include "bridge_slaves/bridge_slave_led_driver.h"
#include "bridge_slaves/bridge_slave_uhk_module.h"
#include "led_pwm.h"

static uint8_t activeLayer = LAYER_ID_BASE;
static uint8_t mouseWheelDivisorCounter = 0;
static uint8_t mouseSpeedAccelDivisorCounter = 0;
static uint8_t mouseSpeed = 3;
static bool wasPreviousMouseActionWheelAction = false;
test_states_t TestStates;

void processMouseAction(key_action_t action)
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

void processTestAction(key_action_t testAction) {
    switch (testAction.test.testAction) {
    case TestAction_DisableUsb:
        if (kStatus_USB_Success != USB_DeviceClassDeinit(CONTROLLER_ID)) {
            return;
        }
        // Make sure we are clocking to the peripheral to ensure there are no bus errors
        if (SIM->SCGC4 & SIM_SCGC4_USBOTG_MASK) {
            NVIC_DisableIRQ(USB0_IRQn);           // Disable the USB interrupt
            NVIC_ClearPendingIRQ(USB0_IRQn);      // Clear any pending interrupts on USB
            SIM->SCGC4 &= ~SIM_SCGC4_USBOTG_MASK; // Turn off clocking to USB
        }
        break;
    case TestAction_DisableI2c:
        TestStates.disableI2c = true;
        break;
    case TestAction_DisableKeyMatrixScan:
        TestStates.disableKeyMatrixScan = true;
        break;
    case TestAction_DisableLedDriverPwm:
        SetLeds(0);
        break;
    case TestAction_DisableLedFetPwm:
        LedPwm_SetBrightness(0);
        UhkModuleStates[0].ledPwmBrightness = 0;
        break;
    case TestAction_DisableLedSdb:
        GPIO_WritePinOutput(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, 0);
        TestStates.disableLedSdb = true;
        break;
    }
}

void UpdateActiveUsbReports() {

    bzero(&UsbMouseReport, sizeof(usb_mouse_report_t));

    uint8_t basicScancodeIndex = 0;
    uint8_t mediaScancodeIndex = 0;
    uint8_t systemScancodeIndex = 0;

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
                    ActiveUsbBasicKeyboardReport->modifiers |= action.keystroke.modifiers;

                    switch (action.keystroke.keystrokeType) {
                        case KEYSTROKE_BASIC:
                            if (basicScancodeIndex >= USB_BASIC_KEYBOARD_MAX_KEYS) {
                                break;
                            }
                            ActiveUsbBasicKeyboardReport->scancodes[basicScancodeIndex++] = action.keystroke.scancode;
                            break;
                        case KEYSTROKE_MEDIA:
                            if (mediaScancodeIndex >= USB_MEDIA_KEYBOARD_MAX_KEYS) {
                                break;
                            }
                            ActiveUsbMediaKeyboardReport->scancodes[mediaScancodeIndex++] = action.keystroke.scancode;
                            break;
                        case KEYSTROKE_SYSTEM:
                            if (systemScancodeIndex >= USB_SYSTEM_KEYBOARD_MAX_KEYS) {
                                break;
                            }
                            ActiveUsbSystemKeyboardReport->scancodes[systemScancodeIndex++] = action.keystroke.scancode;
                            break;
                    }
                    break;
                case KEY_ACTION_MOUSE:
                    processMouseAction(action);
                    break;
                case KEY_ACTION_TEST:
                    processTestAction(action);
                    break;
            }
        }
    }
}
