#include <math.h>
#include "key_action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "keymap.h"
#include "peripherals/test_led.h"
#include "slave_drivers/is31fl3731_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "macros.h"
#include "key_states.h"
#include "right_key_matrix.h"
#include "layer.h"
#include "usb_report_updater.h"
#include "timer.h"
#include "key_debouncer.h"

uint32_t UsbReportUpdateTime = 0;
static uint32_t elapsedTime;

static float mouseMoveSpeed = 0.4;
static float mouseMoveInitialSpeed = 1;
static float mouseMoveAcceleration = 5;
static float mouseMoveMaxSpeed = 5;

static float mouseScrollSpeed = 0.1;
static float mouseScrollMaxSpeed = 0.1;

static float mouseAccelerateFactor = 2;
static float mouseDecelerateFactor = 0.5;

static bool isMouseActionProcessed;
static bool wasMouseActionProcessed;
void processMouseAction(key_action_t *action)
{
    static float mouseMoveCurrentSpeed;

    isMouseActionProcessed = true;
    if (!wasMouseActionProcessed) {
        mouseMoveCurrentSpeed = mouseMoveInitialSpeed;
    }

    if (action->mouse.speedActions) {
        if (action->mouse.speedActions & MouseSpeed_Accelerate) {
            if (mouseMoveSpeed < mouseMoveMaxSpeed) {
                mouseMoveSpeed++;
            }
        } else if (action->mouse.speedActions & MouseSpeed_Decelerate) {
            if (mouseMoveSpeed > 1) {
                mouseMoveSpeed--;
            }
        }
    }

    if (action->mouse.moveActions) {
        mouseMoveCurrentSpeed += mouseMoveAcceleration * elapsedTime / 1000;
        if (mouseMoveCurrentSpeed > mouseMoveMaxSpeed) {
            mouseMoveCurrentSpeed = mouseMoveMaxSpeed;
        }

        uint16_t distance = mouseMoveCurrentSpeed * elapsedTime / 10;

        if (action->mouse.moveActions & MouseMove_Left) {
            ActiveUsbMouseReport->x = -distance;
        } else if (action->mouse.moveActions & MouseMove_Right) {
            ActiveUsbMouseReport->x = distance;
        }

        if (action->mouse.moveActions & MouseMove_Up) {
            ActiveUsbMouseReport->y = -distance;
        } else if (action->mouse.moveActions & MouseMove_Down) {
            ActiveUsbMouseReport->y = distance;
        }
    }

    static float mouseScrollDistanceSum = 0;
    if (action->mouse.scrollActions) {
        mouseScrollDistanceSum += mouseScrollSpeed;
        float mouseScrollDistanceIntegerSum;
        float mouseScrollDistanceFractionSum = modff(mouseScrollDistanceSum, &mouseScrollDistanceIntegerSum);

        if (mouseScrollDistanceIntegerSum) {
            if (action->mouse.scrollActions & MouseScroll_Up) {
                ActiveUsbMouseReport->wheelX = mouseScrollDistanceIntegerSum;
            } else if (action->mouse.scrollActions & MouseScroll_Down) {
                ActiveUsbMouseReport->wheelX = -mouseScrollDistanceIntegerSum;
            }
            mouseScrollDistanceSum = mouseScrollDistanceFractionSum;
        }
    } else {
        mouseScrollDistanceSum = 0;
    }

    ActiveUsbMouseReport->buttons |= action->mouse.buttonActions;
}

static uint8_t basicScancodeIndex = 0;
static uint8_t mediaScancodeIndex = 0;
static uint8_t systemScancodeIndex = 0;

void applyKeyAction(key_state_t *keyState, key_action_t *action)
{
    if (keyState->suppressed) {
        return;
    }

    switch (action->type) {
        case KeyActionType_Keystroke:
            ActiveUsbBasicKeyboardReport->modifiers |= action->keystroke.modifiers;

            switch (action->keystroke.keystrokeType) {
                case KeystrokeType_Basic:
                    if (basicScancodeIndex >= USB_BASIC_KEYBOARD_MAX_KEYS || action->keystroke.scancode == 0) {
                        break;
                    }
                    ActiveUsbBasicKeyboardReport->scancodes[basicScancodeIndex++] = action->keystroke.scancode;
                    break;
                case KeystrokeType_Media:
                    if (mediaScancodeIndex >= USB_MEDIA_KEYBOARD_MAX_KEYS) {
                        break;
                    }
                    ActiveUsbMediaKeyboardReport->scancodes[mediaScancodeIndex++] = action->keystroke.scancode;
                    break;
                case KeystrokeType_System:
                    if (systemScancodeIndex >= USB_SYSTEM_KEYBOARD_MAX_KEYS) {
                        break;
                    }
                    ActiveUsbSystemKeyboardReport->scancodes[systemScancodeIndex++] = action->keystroke.scancode;
                    break;
            }
            break;
        case KeyActionType_Mouse:
            processMouseAction(action);
            break;
        case KeyActionType_SwitchKeymap:
            if (!keyState->previous && keyState->current) {
                SwitchKeymap(action->switchKeymap.keymapId);
            }
            break;
    }
}

static layer_id_t previousLayer = LayerId_Base;
static uint8_t secondaryRoleState = SecondaryRoleState_Released;
static uint8_t secondaryRoleSlotId;
static uint8_t secondaryRoleKeyId;
static secondary_role_t secondaryRole;

void updateActiveUsbReports(void)
{
    wasMouseActionProcessed = isMouseActionProcessed;
    isMouseActionProcessed = false;

    static uint8_t previousModifiers = 0;
    elapsedTime = Timer_GetElapsedTime(&UsbReportUpdateTime);

    basicScancodeIndex = 0;
    mediaScancodeIndex = 0;
    systemScancodeIndex = 0;

    for (uint8_t keyId=0; keyId < RIGHT_KEY_MATRIX_KEY_COUNT; keyId++) {
        KeyStates[SlotId_RightKeyboardHalf][keyId].current = RightKeyMatrix.keyStates[keyId];
    }

    layer_id_t activeLayer = LayerId_Base;
    if (secondaryRoleState == SecondaryRoleState_Triggered && IS_SECONDARY_ROLE_LAYER_SWITCHER(secondaryRole)) {
        activeLayer = SECONDARY_ROLE_LAYER_TO_LAYER_ID(secondaryRole);
    }
    if (activeLayer == LayerId_Base) {
        activeLayer = GetActiveLayer();
    }
    bool suppressKeys = previousLayer != LayerId_Base && activeLayer == LayerId_Base;
    LedDisplay_SetLayer(activeLayer);

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
            key_state_t *keyState = &KeyStates[slotId][keyId];
            key_action_t *action = &CurrentKeymap[activeLayer][slotId][keyId];


            if (keyState->debounceCounter < KEY_DEBOUNCER_TIMEOUT_MSEC) {
                keyState->current = keyState->previous;
            } else if (!keyState->previous && keyState->current) {
                keyState->debounceCounter = 0;
            }

            if (keyState->current) {
                if (suppressKeys) {
                    keyState->suppressed = true;
                }

                if (action->type == KeyActionType_Keystroke && action->keystroke.secondaryRole) {
                    // Press released secondary role key.
                    if (!keyState->previous && action->type == KeyActionType_Keystroke && action->keystroke.secondaryRole && secondaryRoleState == SecondaryRoleState_Released) {
                        secondaryRoleState = SecondaryRoleState_Pressed;
                        secondaryRoleSlotId = slotId;
                        secondaryRoleKeyId = keyId;
                        secondaryRole = action->keystroke.secondaryRole;
                        keyState->suppressed = true;
                    }
                } else {
                    // Trigger secondary role.
                    if (!keyState->previous && secondaryRoleState == SecondaryRoleState_Pressed) {
                        secondaryRoleState = SecondaryRoleState_Triggered;
                    } else {
                        applyKeyAction(keyState, action);
                    }
                }
            } else {
                if (keyState->suppressed) {
                    keyState->suppressed = false;
                }

                // Release secondary role key.
                if (keyState->previous && secondaryRoleSlotId == slotId && secondaryRoleKeyId == keyId) {
                    // Trigger primary role.
                    if (secondaryRoleState == SecondaryRoleState_Pressed) {
                        applyKeyAction(keyState, action);
                    }
                    secondaryRoleState = SecondaryRoleState_Released;
                }
            }

            keyState->previous = keyState->current;
        }
    }

    // When a layer switcher key gets pressed along with another key that produces some modifiers
    // and the accomanying key gets released then keep the related modifiers active a long as the
    // layer switcher key stays pressed.  Useful for Alt+Tab keymappings and the like.
    if (activeLayer != LayerId_Base && activeLayer == PreviousHeldLayer && basicScancodeIndex == 0) {
        ActiveUsbBasicKeyboardReport->modifiers |= previousModifiers;
    }

    if (secondaryRoleState == SecondaryRoleState_Triggered && IS_SECONDARY_ROLE_MODIFIER(secondaryRole)) {
        ActiveUsbBasicKeyboardReport->modifiers |= SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secondaryRole);
    }

    previousModifiers = ActiveUsbBasicKeyboardReport->modifiers;
    previousLayer = activeLayer;
}

bool UsbBasicKeyboardReportEverSent = false;
bool UsbMediaKeyboardReportEverSent = false;
bool UsbSystemKeyboardReportEverSent = false;
bool UsbMouseReportEverSentEverSent = false;

void UpdateUsbReports(void)
{
    if (IsUsbBasicKeyboardReportSent) {
        UsbBasicKeyboardReportEverSent = true;
    }
    if (IsUsbMediaKeyboardReportSent) {
        UsbMediaKeyboardReportEverSent = true;
    }
    if (IsUsbSystemKeyboardReportSent) {
        UsbSystemKeyboardReportEverSent = true;
    }
    if (IsUsbMouseReportSent) {
        UsbMouseReportEverSentEverSent = true;
    }

    bool areUsbReportsSent = true;
    if (UsbBasicKeyboardReportEverSent) {
        areUsbReportsSent &= IsUsbBasicKeyboardReportSent;
    }
    if (UsbMediaKeyboardReportEverSent) {
        areUsbReportsSent &= IsUsbMediaKeyboardReportSent;
    }
    if (UsbSystemKeyboardReportEverSent) {
        areUsbReportsSent &= IsUsbSystemKeyboardReportSent;
    }
    if (UsbMouseReportEverSentEverSent) {
        areUsbReportsSent &= IsUsbMouseReportSent;
    }
    if (!areUsbReportsSent) {
        return;
    }

    ResetActiveUsbBasicKeyboardReport();
    ResetActiveUsbMediaKeyboardReport();
    ResetActiveUsbSystemKeyboardReport();
    ResetActiveUsbMouseReport();

    updateActiveUsbReports();

    SwitchActiveUsbBasicKeyboardReport();
    SwitchActiveUsbMediaKeyboardReport();
    SwitchActiveUsbSystemKeyboardReport();
    SwitchActiveUsbMouseReport();

    IsUsbBasicKeyboardReportSent = false;
    IsUsbMediaKeyboardReportSent = false;
    IsUsbSystemKeyboardReportSent = false;
    IsUsbMouseReportSent = false;
}
