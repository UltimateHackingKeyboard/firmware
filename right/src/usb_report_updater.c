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
#include "right_key_matrix.h"
#include "layer.h"
#include "usb_report_updater.h"
#include "timer.h"
#include "config_parser/parse_keymap.h"
#include "usb_commands/usb_command_get_debug_buffer.h"
#include "arduino_hid/ConsumerAPI.h"

static uint32_t mouseUsbReportUpdateTime = 0;
static uint32_t mouseElapsedTime;

uint16_t DoubleTapSwitchLayerTimeout = 300;
static uint16_t DoubleTapSwitchLayerReleaseTimeout = 200;

static bool activeMouseStates[ACTIVE_MOUSE_STATES_COUNT];
bool TestUsbStack = false;

volatile uint8_t UsbReportUpdateSemaphore = 0;

mouse_kinetic_state_t MouseMoveState = {
    .isScroll = false,
    .upState = SerializedMouseAction_MoveUp,
    .downState = SerializedMouseAction_MoveDown,
    .leftState = SerializedMouseAction_MoveLeft,
    .rightState = SerializedMouseAction_MoveRight,
    .intMultiplier = 25,
    .initialSpeed = 5,
    .acceleration = 35,
    .deceleratedSpeed = 10,
    .baseSpeed = 40,
    .acceleratedSpeed = 80,
};

mouse_kinetic_state_t MouseScrollState = {
    .isScroll = true,
    .upState = SerializedMouseAction_ScrollDown,
    .downState = SerializedMouseAction_ScrollUp,
    .leftState = SerializedMouseAction_ScrollLeft,
    .rightState = SerializedMouseAction_ScrollRight,
    .intMultiplier = 1,
    .initialSpeed = 20,
    .acceleration = 20,
    .deceleratedSpeed = 10,
    .baseSpeed = 20,
    .acceleratedSpeed = 50,
};

static void processMouseKineticState(mouse_kinetic_state_t *kineticState)
{
    float initialSpeed = kineticState->intMultiplier * kineticState->initialSpeed;
    float acceleration = kineticState->intMultiplier * kineticState->acceleration;
    float deceleratedSpeed = kineticState->intMultiplier * kineticState->deceleratedSpeed;
    float baseSpeed = kineticState->intMultiplier * kineticState->baseSpeed;
    float acceleratedSpeed = kineticState->intMultiplier * kineticState->acceleratedSpeed;

    if (!kineticState->wasMoveAction && !activeMouseStates[SerializedMouseAction_Decelerate]) {
        kineticState->currentSpeed = initialSpeed;
    }

    bool isMoveAction = activeMouseStates[kineticState->upState] ||
                        activeMouseStates[kineticState->downState] ||
                        activeMouseStates[kineticState->leftState] ||
                        activeMouseStates[kineticState->rightState];

    mouse_speed_t mouseSpeed = MouseSpeed_Normal;
    if (activeMouseStates[SerializedMouseAction_Accelerate]) {
        kineticState->targetSpeed = acceleratedSpeed;
        mouseSpeed = MouseSpeed_Accelerated;
    } else if (activeMouseStates[SerializedMouseAction_Decelerate]) {
        kineticState->targetSpeed = deceleratedSpeed;
        mouseSpeed = MouseSpeed_Decelerated;
    } else if (isMoveAction) {
        kineticState->targetSpeed = baseSpeed;
    }

    if (mouseSpeed == MouseSpeed_Accelerated || (kineticState->wasMoveAction && isMoveAction && (kineticState->prevMouseSpeed != mouseSpeed))) {
        kineticState->currentSpeed = kineticState->targetSpeed;
    }

    if (isMoveAction) {
        if (kineticState->currentSpeed < kineticState->targetSpeed) {
            kineticState->currentSpeed += acceleration * (float)mouseElapsedTime / 1000.0f;
            if (kineticState->currentSpeed > kineticState->targetSpeed) {
                kineticState->currentSpeed = kineticState->targetSpeed;
            }
        } else {
            kineticState->currentSpeed -= acceleration * (float)mouseElapsedTime / 1000.0f;
            if (kineticState->currentSpeed < kineticState->targetSpeed) {
                kineticState->currentSpeed = kineticState->targetSpeed;
            }
        }

        float distance = kineticState->currentSpeed * (float)mouseElapsedTime / 1000.0f;


        if (kineticState->isScroll && !kineticState->wasMoveAction) {
            kineticState->xSum = 0;
            kineticState->ySum = 0;
        }

        // Update horizontal state

        bool horizontalMovement = true;
        if (activeMouseStates[kineticState->leftState]) {
            kineticState->xSum -= distance;
        } else if (activeMouseStates[kineticState->rightState]) {
            kineticState->xSum += distance;
        } else {
            horizontalMovement = false;
        }

        float xSumInt;
        float xSumFrac = modff(kineticState->xSum, &xSumInt);
        kineticState->xSum = xSumFrac;
        kineticState->xOut = xSumInt;

        if (kineticState->isScroll && !kineticState->wasMoveAction && kineticState->xOut == 0 && horizontalMovement) {
            kineticState->xOut = kineticState->xSum ? copysignf(1.0, kineticState->xSum) : 0;
            kineticState->xSum = 0;
        }

        // Update vertical state

        bool verticalMovement = true;
        if (activeMouseStates[kineticState->upState]) {
            kineticState->ySum -= distance;
        } else if (activeMouseStates[kineticState->downState]) {
            kineticState->ySum += distance;
        } else {
            verticalMovement = false;
        }

        float ySumInt;
        float ySumFrac = modff(kineticState->ySum, &ySumInt);
        kineticState->ySum = ySumFrac;
        kineticState->yOut = ySumInt;

        if (kineticState->isScroll && !kineticState->wasMoveAction && kineticState->yOut == 0 && verticalMovement) {
            kineticState->yOut = kineticState->ySum ? copysignf(1.0, kineticState->ySum) : 0;
            kineticState->ySum = 0;
        }
    } else {
        kineticState->currentSpeed = 0;
    }

    kineticState->prevMouseSpeed = mouseSpeed;
    kineticState->wasMoveAction = isMoveAction;
}

static void processMouseActions()
{
    mouseElapsedTime = Timer_GetElapsedTimeAndSetCurrent(&mouseUsbReportUpdateTime);

    processMouseKineticState(&MouseMoveState);
    ActiveUsbMouseReport->x = MouseMoveState.xOut;
    ActiveUsbMouseReport->y = MouseMoveState.yOut;
    MouseMoveState.xOut = 0;
    MouseMoveState.yOut = 0;

    processMouseKineticState(&MouseScrollState);
    ActiveUsbMouseReport->wheelX = MouseScrollState.xOut;
    ActiveUsbMouseReport->wheelY = MouseScrollState.yOut;
    MouseScrollState.xOut = 0;
    MouseScrollState.yOut = 0;

//  The following line makes the firmware crash for some reason:
//  SetDebugBufferFloat(60, mouseScrollState.currentSpeed);
//  TODO: Figure out why.
//  Oddly, the following line (which is the inlined version of the above) works:
//  *(float*)(DebugBuffer + 60) = mouseScrollState.currentSpeed;
//  The value parameter of SetDebugBufferFloat() seems to be the culprit because
//  if it's not used within the function it doesn't crash anymore.

    if (activeMouseStates[SerializedMouseAction_LeftClick]) {
        ActiveUsbMouseReport->buttons |= MouseButton_Left;
    }
    if (activeMouseStates[SerializedMouseAction_MiddleClick]) {
        ActiveUsbMouseReport->buttons |= MouseButton_Middle;
    }
    if (activeMouseStates[SerializedMouseAction_RightClick]) {
        ActiveUsbMouseReport->buttons |= MouseButton_Right;
    }
}

static layer_id_t previousLayer = LayerId_Base;

static void handleSwitchLayerAction(key_state_t *keyState, key_action_t *action)
{
    static key_state_t *doubleTapSwitchLayerKey;
    static uint32_t doubleTapSwitchLayerStartTime;
    static uint32_t doubleTapSwitchLayerTriggerTime;
    static bool isLayerDoubleTapToggled;

    if (doubleTapSwitchLayerKey && doubleTapSwitchLayerKey != keyState && !keyState->previous) {
        doubleTapSwitchLayerKey = NULL;
    }

    if (action->type != KeyActionType_SwitchLayer) {
        return;
    }

    if (!keyState->previous && isLayerDoubleTapToggled && ToggledLayer == action->switchLayer.layer) {
        ToggledLayer = LayerId_Base;
        isLayerDoubleTapToggled = false;
    }

    if (keyState->previous && doubleTapSwitchLayerKey == keyState &&
        Timer_GetElapsedTime(&doubleTapSwitchLayerTriggerTime) > DoubleTapSwitchLayerReleaseTimeout)
    {
        ToggledLayer = LayerId_Base;
    }

    if (!keyState->previous && previousLayer == LayerId_Base && action->switchLayer.mode == SwitchLayerMode_HoldAndDoubleTapToggle) {
        if (doubleTapSwitchLayerKey && Timer_GetElapsedTimeAndSetCurrent(&doubleTapSwitchLayerStartTime) < DoubleTapSwitchLayerTimeout) {
            ToggledLayer = action->switchLayer.layer;
            isLayerDoubleTapToggled = true;
            doubleTapSwitchLayerTriggerTime = CurrentTime;
        } else {
            doubleTapSwitchLayerKey = keyState;
        }
        doubleTapSwitchLayerStartTime = CurrentTime;
    }
}

static uint8_t basicScancodeIndex = 0;
static uint8_t mediaScancodeIndex = 0;
static uint8_t systemScancodeIndex = 0;

static uint8_t stickyModifiers;


static void applyKeyAction(key_state_t *keyState, key_action_t *action)
{
    if (keyState->suppressed) {
//        return;
    }

    handleSwitchLayerAction(keyState, action);

    switch (action->type) {
        case KeyActionType_Keystroke:
            if (action->keystroke.scancode) {
                if (!keyState->previous) {
                    stickyModifiers = action->keystroke.modifiers;
                }
            } else {
                ActiveUsbBasicKeyboardReport->modifiers |= action->keystroke.modifiers;
            }
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
            if (!keyState->previous) {
                stickyModifiers = 0;
            }
            activeMouseStates[action->mouseAction] = true;
            break;
        case KeyActionType_SwitchLayer:
            // Handled by handleSwitchLayerAction()
            break;
        case KeyActionType_SwitchKeymap:
            if (!keyState->previous) {
                stickyModifiers = 0;
                SwitchKeymapById(action->switchKeymap.keymapId);
            }
            break;
        case KeyActionType_PlayMacro:
            if (!keyState->previous) {
                stickyModifiers = 0;
                Macros_StartMacro(action->playMacro.macroId);
            }
            break;
    }
}

static int IndexOf(active_key_t *keys, active_key_t *record, uint8_t size) {
    for (uint8_t i = 0; i < size; ++i) {
        if (keys[i].slotId == record->slotId && keys[i].keyId == record->keyId) {
            return (int) i;
        }
    }
    return -1;
}

static void RemoveAt(active_key_t *keys, uint8_t index, uint8_t size) {
    for (uint8_t i = index; i < size - 1; ++i) {
        keys[i] = keys[i + 1];
    }

    // TODO - figure out why this is needed, possibly there's a bug in array operations
    (&keys[index])->secondaryRoleEnqueueTime = -1;
    (&keys[index])->secondaryRoleState = SecondaryRoleState_Released;
}

static void InsertAt(active_key_t *keys, active_key_t *newActiveKey, uint8_t index, uint8_t size) {
    for (uint8_t i = size + 1; i > index; --i) {
        keys[i] = keys[i - 1];
    }
    keys[index] = *newActiveKey;
}

static bool Remove(active_key_t *keys, active_key_t *record, uint8_t size) {
    int index = IndexOf(keys, record, size);
    if (index >= 0) {
        RemoveAt(keys, (uint8_t) index, size);
        return true;
    }
    return false;
}

static void mitigateBouncing(key_state_t *keyState) {
    uint8_t debounceTimeOut = (keyState->previous ? DebounceTimePress : DebounceTimeRelease);
    if (keyState->debouncing) {
        if ((uint8_t)(CurrentTime - keyState->timestamp) > debounceTimeOut) {
            keyState->debouncing = false;
        } else {
            keyState->current = keyState->previous;
        }
    } else if (keyState->previous != keyState->current) {
        keyState->timestamp = CurrentTime;
        keyState->debouncing = true;
    }
}

// TODO - this array should be much smaller
static active_key_t pendingSecondaryRoleKeys[100];
static uint8_t pendingSecondaryRoleKeyCount = 0;

static void updateActiveUsbReports(void)
{
    if (MacroPlaying) {
        Macros_ContinueMacro();
        memcpy(ActiveUsbMouseReport, &MacroMouseReport, sizeof MacroMouseReport);
        memcpy(ActiveUsbBasicKeyboardReport, &MacroBasicKeyboardReport, sizeof MacroBasicKeyboardReport);
        memcpy(ActiveUsbMediaKeyboardReport, &MacroMediaKeyboardReport, sizeof MacroMediaKeyboardReport);
        memcpy(ActiveUsbSystemKeyboardReport, &MacroSystemKeyboardReport, sizeof MacroSystemKeyboardReport);
        return;
    }

    memset(activeMouseStates, 0, ACTIVE_MOUSE_STATES_COUNT);

    basicScancodeIndex = 0;
    mediaScancodeIndex = 0;
    systemScancodeIndex = 0;

    layer_id_t activeLayer = LayerId_Base;

    uint8_t activeKeysCount = 0;
    // TODO - this array should be much smaller
    active_key_t activeKeyStates[100];

    for (uint8_t i = 0; i < pendingSecondaryRoleKeyCount; ++i) {
        active_key_t *secondaryRoleKey = &pendingSecondaryRoleKeys[i];
        if (secondaryRoleKey->secondaryRoleState == SecondaryRoleState_Triggered) {
            uint8_t secondaryRole = secondaryRoleKey->action->keystroke.secondaryRole;
            bool isActionLayerSwitch = IS_SECONDARY_ROLE_LAYER_SWITCHER(secondaryRole);
            if (isActionLayerSwitch) {
                activeLayer = SECONDARY_ROLE_LAYER_TO_LAYER_ID(secondaryRole);
                break;
            }
        }
    }

    if (activeLayer == LayerId_Base) {
        activeLayer = GetActiveLayer();
    }

    bool layerChanged = previousLayer != activeLayer;
    if (layerChanged) {
        stickyModifiers = 0;
    }

    LedDisplay_SetLayer(activeLayer);
    bool isPrimaryRoleKeyOnlyKeyPressed = false;
    for (uint8_t slotId = 0; slotId < SLOT_COUNT; slotId++) {
        for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];

            mitigateBouncing(keyState);

            if (keyState->current && !keyState->previous) {
                if (SleepModeActive) {
                    WakeUpHost();
                }
            }

            if (keyState->current) {
                active_key_t activeKey;
                activeKey.action = &CurrentKeymap[activeLayer][slotId][keyId];
                activeKey.state = keyState;
                activeKey.keyId = keyId;
                activeKey.slotId = slotId;

                bool hasSecondaryRole = activeKey.action->type == KeyActionType_Keystroke && activeKey.action->keystroke.secondaryRole;
                int pendingIndex = IndexOf(pendingSecondaryRoleKeys, &activeKey, pendingSecondaryRoleKeyCount);
                //
                if (hasSecondaryRole) {
                    if (pendingIndex < 0 && !keyState->suppressed) {
                        activeKey.secondaryRoleEnqueueTime = CurrentTime;
                        pendingSecondaryRoleKeys[pendingSecondaryRoleKeyCount++] = activeKey;
                    }
                } else {
                    if (pendingIndex < 0) {
                        activeKeyStates[activeKeysCount++] = activeKey;
                    }
                }

                if (!hasSecondaryRole && activeKey.action->keystroke.scancode) {
                    isPrimaryRoleKeyOnlyKeyPressed = true;
                }
            } else {
                keyState->suppressed = false;
                keyState->previous = false;
            }
        }
    }

    // TODO - go backwards here or schedule the secondary roles in a stack fashion
    for (uint8_t i = 0; i < pendingSecondaryRoleKeyCount;) {
        active_key_t *activeKey = &pendingSecondaryRoleKeys[i];
        key_state_t *state = activeKey->state;

        if (state->suppressed) {
            ++i;
            continue;
        }

        int timeout = 120;
        bool secondaryRoleTimeoutElapsed = (int)(CurrentTime - activeKey->secondaryRoleEnqueueTime) > timeout;
        bool secondaryRoleDiscardTimeoutElapsed = (int)(CurrentTime - activeKey->secondaryRoleEnqueueTime) > 2 * timeout;

        bool isEvicted = false;

        if (!state->current) {
            // the secondary key has been released and the time out not yet elapsed -> trigger the first role
            if (!secondaryRoleDiscardTimeoutElapsed) {
                InsertAt(activeKeyStates, activeKey, 0, activeKeysCount++);
            }

            // TODO - get to know the shenanigans of the sticky modifiers better and try to optimise this
            stickyModifiers = activeKey->action->keystroke.modifiers;
            isEvicted = Remove(pendingSecondaryRoleKeys, activeKey, pendingSecondaryRoleKeyCount--);
        }

        if (state->current) {
            state->previous = true;
            // either only secondary role keys are pressed or the timeout elapsed
            if (secondaryRoleTimeoutElapsed && !state->suppressed) {
                activeKey->secondaryRoleState = SecondaryRoleState_Triggered;

                if (activeKey->action->type == KeyActionType_Keystroke) {
                    uint8_t secondaryRole = activeKey->action->keystroke.secondaryRole;
                    if (IS_SECONDARY_ROLE_MODIFIER(secondaryRole)) {
                        ActiveUsbBasicKeyboardReport->modifiers |= SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secondaryRole);
                    }
                }
            } else {
                // secondary role was pressed, but some other key was sent too early after
                // then trigger the primary role of the key and forget about it
                if (isPrimaryRoleKeyOnlyKeyPressed) {
                    state->suppressed = true;
                    InsertAt(activeKeyStates, activeKey, 0, activeKeysCount++);
                    isEvicted = Remove(pendingSecondaryRoleKeys, activeKey, pendingSecondaryRoleKeyCount--);
                    // this will discard the sticky modifiers thing in the apply function ><
                    // TODO - get to know the shenanigans of the sticky modifiers better and try to optimise this
                    stickyModifiers = activeKey->action->keystroke.modifiers;

                }
            }
        }

        if (!isEvicted) {
            ++i;
        }
    }

    for (uint8_t i = 0; i < activeKeysCount; ++i) {
        active_key_t *activeKey = &activeKeyStates[i];
        key_state_t *keyState = activeKey->state;
        key_action_t *action = activeKey->action;
        applyKeyAction(keyState, action);

        keyState->previous = keyState->current;
    }

    processMouseActions();

    // When a layer switcher key gets pressed along with another key that produces some modifiers
    // and the accomanying key gets released then keep the related modifiers active a long as the
    // layer switcher key stays pressed.  Useful for Alt+Tab keymappings and the like.
    ActiveUsbBasicKeyboardReport->modifiers |= stickyModifiers;

    previousLayer = activeLayer;
}

uint32_t UsbReportUpdateCounter;

void UpdateUsbReports(void)
{
    static uint32_t lastUpdateTime;

    for (uint8_t keyId = 0; keyId < RIGHT_KEY_MATRIX_KEY_COUNT; keyId++) {
        KeyStates[SlotId_RightKeyboardHalf][keyId].current = RightKeyMatrix.keyStates[keyId];
    }

    if (UsbReportUpdateSemaphore && !SleepModeActive) {
        if (Timer_GetElapsedTime(&lastUpdateTime) < USB_SEMAPHORE_TIMEOUT) {
            return;
        } else {
            UsbReportUpdateSemaphore = 0;
        }
    }

    lastUpdateTime = CurrentTime;
    UsbReportUpdateCounter++;

    ResetActiveUsbBasicKeyboardReport();
    ResetActiveUsbMediaKeyboardReport();
    ResetActiveUsbSystemKeyboardReport();
    ResetActiveUsbMouseReport();

    updateActiveUsbReports();

    bool HasUsbBasicKeyboardReportChanged = memcmp(ActiveUsbBasicKeyboardReport, GetInactiveUsbBasicKeyboardReport(), sizeof(usb_basic_keyboard_report_t)) != 0;
    bool HasUsbMediaKeyboardReportChanged = memcmp(ActiveUsbMediaKeyboardReport, GetInactiveUsbMediaKeyboardReport(), sizeof(usb_media_keyboard_report_t)) != 0;
    bool HasUsbSystemKeyboardReportChanged = memcmp(ActiveUsbSystemKeyboardReport, GetInactiveUsbSystemKeyboardReport(), sizeof(usb_system_keyboard_report_t)) != 0;
    bool HasUsbMouseReportChanged = memcmp(ActiveUsbMouseReport, GetInactiveUsbMouseReport(), sizeof(usb_mouse_report_t)) != 0;

    if (HasUsbBasicKeyboardReportChanged) {
        usb_status_t status = UsbBasicKeyboardAction();
        if (status == kStatus_USB_Success) {
            UsbReportUpdateSemaphore |= 1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX;
        }
    }

    if (HasUsbMediaKeyboardReportChanged) {
        usb_status_t status = UsbMediaKeyboardAction();
        if (status == kStatus_USB_Success) {
            UsbReportUpdateSemaphore |= 1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX;
        }
    }

    if (HasUsbSystemKeyboardReportChanged) {
        usb_status_t status = UsbSystemKeyboardAction();
        if (status == kStatus_USB_Success) {
            UsbReportUpdateSemaphore |= 1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX;
        }
    }

    // Send out the mouse position and wheel values continuously if the report is not zeros, but only send the mouse button states when they change.
    if (HasUsbMouseReportChanged || ActiveUsbMouseReport->x || ActiveUsbMouseReport->y ||
            ActiveUsbMouseReport->wheelX || ActiveUsbMouseReport->wheelY) {
        usb_status_t status = UsbMouseAction();
        if (status == kStatus_USB_Success) {
            UsbReportUpdateSemaphore |= 1 << USB_MOUSE_INTERFACE_INDEX;
        }
    }
}
