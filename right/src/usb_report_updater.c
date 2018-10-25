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
    handleSwitchLayerAction(keyState, action);

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
            if (!keyState->previous) {
                stickyModifiers = 0;
            }
            activeMouseStates[action->mouseAction] = true;
            break;
        case KeyActionType_SwitchLayer:
            // Handled by handleSwitchLayerAction()
            break;
        case KeyActionType_SwitchKeymap:
                stickyModifiers = 0;
                SwitchKeymapById(action->switchKeymap.keymapId);
            break;
        case KeyActionType_PlayMacro:
                stickyModifiers = 0;
                Macros_StartMacro(action->playMacro.macroId);
            break;
    }
}

static int IndexOf(pending_key_t *keys, key_ref_t *key, uint8_t size) {
    for (uint8_t i = 0; i < size; ++i) {
        if (keys[i].ref.slotId == key->slotId && keys[i].ref.keyId == key->keyId) {
            return (int) i;
        }
    }
    return -1;
}

static void RemoveAt(pending_key_t *keys, uint8_t index, uint8_t size) {
    for (int i = index; i < size - 1; ++i) {
        keys[i] = keys[i + 1];
    }
}

static void InsertAt(pending_key_t *keys, pending_key_t *newActiveKey, uint8_t index, uint8_t size) {
    for (int i = size - 1; i > index; --i) {
        keys[i] = keys[i - 1];
    }
    keys[index] = *newActiveKey;
}

static bool Remove(pending_key_t *keys, key_ref_t *record, uint8_t size) {
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

static pending_key_t pendingModifiers[100];
static uint8_t pendingModifierCount = 0;

static pending_key_t pendingActions[100];
static uint8_t pendingActionCount = 0;

static inline uint8_t secondaryRole(key_action_t *action) {
    return action->type == KeyActionType_Keystroke ? action->keystroke.secondaryRole : 0;
}

static uint8_t applySecondaryRoleOf(key_ref_t *keyRef, uint8_t layer) {
    uint8_t secRole = secondaryRole(&CurrentKeymap[layer][keyRef->slotId][keyRef->keyId]);

    bool isActionLayerSwitch = IS_SECONDARY_ROLE_LAYER_SWITCHER(secRole);
    if (isActionLayerSwitch) {
        return SECONDARY_ROLE_LAYER_TO_LAYER_ID(secRole);
    } else if (IS_SECONDARY_ROLE_MODIFIER(secRole)) {
        ActiveUsbBasicKeyboardReport->modifiers |= SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secRole);
    }
    return layer;
}

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

    bool pendingActionKeyReleaseDetected = false;
    uint8_t pressedKeyAmount = 0;

    key_ref_t pressedKeys[100];
    for (uint8_t slotId = 0; slotId < SLOT_COUNT; slotId++) {
        for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];

            mitigateBouncing(keyState);

            if (keyState->current && !keyState->previous) {
                if (SleepModeActive) {
                    WakeUpHost();
                }
            }

            key_ref_t ref = {.keyId = keyId, .slotId = slotId, .keyState = keyState};
            if (keyState->current) {
                pressedKeys[pressedKeyAmount++] = ref;
            }

            keyState->previous = keyState->current;
        }
    }

    for (uint8_t i = 0; i < pendingActionCount; ++i) {
        pending_key_t *key = &pendingActions[i];
        if (!key->ref.keyState->current) {
            if (pendingModifierCount > 0) {
                pendingActionKeyReleaseDetected = true;
                key->ref.keyState->previous = false;
                key->ref.keyState->current = true;
            } else {
                Remove(pendingActions, &key->ref, pendingActionCount--);
            }
        }
    }

    layer_id_t activeLayer = LayerId_Base;
    bool activeModifierDetected = false;
    for (uint8_t i = 0; i < pendingModifierCount;) {
        pending_key_t *pendingModifier = &pendingModifiers[i];
        bool timeoutElapsed = (CurrentTime - pendingModifier->enqueueTime) > 500;

        if (pendingModifier->ref.keyState->current) {
            pendingModifier->activated |= timeoutElapsed || pendingActionKeyReleaseDetected;
            if (pendingModifier->activated) {
                activeLayer = applySecondaryRoleOf(&pendingModifier->ref, activeLayer);
                activeModifierDetected = true;
            }
            ++i;
        } else {
            Remove(pendingModifiers, &pendingModifier->ref, pendingModifierCount);
            --pendingModifierCount;
            if (!timeoutElapsed && !pendingModifier->activated) {
                InsertAt(pendingActions, pendingModifier, 0, pendingActionCount);
                pendingModifier->ref.keyState->current = true;
                ++pendingActionCount;
                pendingActionKeyReleaseDetected = true;
            }
        }
    }

    if (activeLayer == LayerId_Base) {
        activeLayer = GetActiveLayer();
    }

    LedDisplay_SetLayer(activeLayer);

    for (uint8_t i = 0; i < pressedKeyAmount; ++i) {
        key_ref_t *ref = &pressedKeys[i];
        key_action_t *action = &CurrentKeymap[activeLayer][ref->slotId][ref->keyId];

        pending_key_t key = {
                .activated = false,
                .enqueueTime = CurrentTime,
                .ref = *ref,
                .action = action
        };

        bool hasSecondaryRole = secondaryRole(action);
        bool notRegisteredAsModifier = IndexOf(pendingModifiers, ref, pendingModifierCount) < 0;
        if (hasSecondaryRole && notRegisteredAsModifier) {
            InsertAt(pendingModifiers, &key, pendingModifierCount, pendingModifierCount);
            ++pendingModifierCount;
        } else if (notRegisteredAsModifier && IndexOf(pendingActions, ref, pendingActionCount) < 0) {
            InsertAt(pendingActions, &key, pendingActionCount, pendingActionCount);
            ++pendingActionCount;
        }
    }

    for (uint8_t i = 0; i < pendingActionCount; ) {
        pending_key_t *key = &pendingActions[i];
        key_action_t *action = &CurrentKeymap[activeLayer][key->ref.slotId][key->ref.keyId];
        key_state_t *keyState = key->ref.keyState;

        bool blockedByModifiers = (pendingModifierCount > 0 && !activeModifierDetected);
        bool shouldApply = blockedByModifiers != keyState->current;
        if (shouldApply) {
            applyKeyAction(keyState, action);
            Remove(pendingActions, &key->ref, pendingActionCount);
            --pendingActionCount;
        } else {
            ++i;
        }
    }

    processMouseActions();

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

