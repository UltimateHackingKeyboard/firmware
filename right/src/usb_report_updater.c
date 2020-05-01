#include <math.h>
#include "key_action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "keymap.h"
#include "peripherals/test_led.h"
#include "slave_drivers/is31fl37xx_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "macros.h"
#include "key_states.h"
#include "right_key_matrix.h"
#include "layer.h"
#include "usb_report_updater.h"
#include "timer.h"
#include "config_parser/parse_keymap.h"
#include "usb_commands/usb_command_get_debug_buffer.h"
#include "arduino_hid/ConsumerAPI.h"
#include "macro_recorder.h"
#include "macro_shortcut_parser.h"
#include "postponer.h"
#include "secondary_role_driver.h"
#include "slave_drivers/touchpad_driver.h"

static uint32_t mouseUsbReportUpdateTime = 0;
static uint32_t mouseElapsedTime;

uint16_t DoubleTapSwitchLayerTimeout = 300;
static uint16_t DoubleTapSwitchLayerReleaseTimeout = 200;

static bool toggledMouseStates[ACTIVE_MOUSE_STATES_COUNT];
static bool activeMouseStates[ACTIVE_MOUSE_STATES_COUNT];
bool TestUsbStack = false;
static key_action_t actionCache[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

volatile uint8_t UsbReportUpdateSemaphore = 0;

uint8_t HardwareModifierState;
uint8_t HardwareModifierStatePrevious;
bool SuppressMods = false;
bool SuppressKeys = false;
bool StickyModifiersEnabled = true;//TODO: refactor this

bool CompensateDiagonalSpeed = false;

key_state_t* EmergencyKey = NULL;

mouse_kinetic_state_t MouseMoveState = {
    .isScroll = false,
    .upState = SerializedMouseAction_MoveUp,
    .downState = SerializedMouseAction_MoveDown,
    .leftState = SerializedMouseAction_MoveLeft,
    .rightState = SerializedMouseAction_MoveRight,
    .verticalStateSign = 0,
    .horizontalStateSign = 0,
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
    .verticalStateSign = 0,
    .horizontalStateSign = 0,
    .intMultiplier = 1,
    .initialSpeed = 20,
    .acceleration = 20,
    .deceleratedSpeed = 10,
    .baseSpeed = 20,
    .acceleratedSpeed = 50,
};

void ToggleMouseState(serialized_mouse_action_t action, bool activate)
{
    toggledMouseStates[action] = activate;
}

static void updateOneDirectionSign(int8_t* sign, int8_t expectedSign, uint8_t expectedState, uint8_t otherState) {
    if (*sign == expectedSign && !activeMouseStates[expectedState]) {
        *sign = activeMouseStates[otherState] ? -expectedSign : 0;
    }
}

// Assume that mouse movement key has been just released. In that case check if there is another key which keeps the state active.
// If not, check whether the other direction state is active and either flip movement direction or zero the state.
static void updateDirectionSigns(mouse_kinetic_state_t *kineticState) {
    updateOneDirectionSign(&kineticState->horizontalStateSign, -1, kineticState->leftState, kineticState->rightState);
    updateOneDirectionSign(&kineticState->horizontalStateSign,  1, kineticState->rightState, kineticState->leftState);
    updateOneDirectionSign(&kineticState->verticalStateSign, -1, kineticState->upState, kineticState->downState);
    updateOneDirectionSign(&kineticState->verticalStateSign,  1, kineticState->downState, kineticState->upState);
}

// Called on keydown of mouse action. Direction signs ensure that the last pressed action always takes precedence, and therefore
// have to be updated statefully.
static void activateDirectionSigns(uint8_t state) {
    switch (state) {
    case SerializedMouseAction_MoveUp:
        MouseMoveState.verticalStateSign = -1;
        break;
    case SerializedMouseAction_MoveDown:
        MouseMoveState.verticalStateSign = 1;
        break;
    case SerializedMouseAction_MoveLeft:
        MouseMoveState.horizontalStateSign = -1;
        break;
    case SerializedMouseAction_MoveRight:
        MouseMoveState.horizontalStateSign = 1;
        break;
    case SerializedMouseAction_ScrollUp:
        MouseScrollState.verticalStateSign = 1;
        break;
    case SerializedMouseAction_ScrollDown:
        MouseScrollState.verticalStateSign = -1;
        break;
    case SerializedMouseAction_ScrollLeft:
        MouseScrollState.horizontalStateSign = -1;
        break;
    case SerializedMouseAction_ScrollRight:
        MouseScrollState.horizontalStateSign = 1;
        break;
    }
}

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

        // Update travelled distances

        updateDirectionSigns(kineticState);

        if ( kineticState->horizontalStateSign != 0 && kineticState->verticalStateSign != 0 && CompensateDiagonalSpeed ) {
            distance /= 1.41f;
        }
        kineticState->xSum += distance * kineticState->horizontalStateSign;
        kineticState->ySum += distance * kineticState->verticalStateSign;

        // Update horizontal state

        bool horizontalMovement = kineticState->horizontalStateSign != 0;

        float xSumInt;
        float xSumFrac = modff(kineticState->xSum, &xSumInt);
        kineticState->xSum = xSumFrac;
        kineticState->xOut = xSumInt;

        // Handle the first scroll tick.
        if (kineticState->isScroll && !kineticState->wasMoveAction && kineticState->xOut == 0 && horizontalMovement) {
            kineticState->xOut = activeMouseStates[kineticState->leftState] ? -1 : 1;
            kineticState->xSum = 0;
        }

        // Update vertical state

        bool verticalMovement = kineticState->verticalStateSign != 0;

        float ySumInt;
        float ySumFrac = modff(kineticState->ySum, &ySumInt);
        kineticState->ySum = ySumFrac;
        kineticState->yOut = ySumInt;

        // Handle the first scroll tick.
        if (kineticState->isScroll && !kineticState->wasMoveAction && kineticState->yOut == 0 && verticalMovement) {
            kineticState->yOut = activeMouseStates[kineticState->upState] ? -1 : 1;
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

    ActiveUsbMouseReport->x += TouchpadUsbMouseReport.x;
    ActiveUsbMouseReport->y += TouchpadUsbMouseReport.y;
    TouchpadUsbMouseReport.x = 0;
    TouchpadUsbMouseReport.y = 0;

    for (uint8_t moduleId=0; moduleId<UHK_MODULE_MAX_COUNT; moduleId++) {
        uhk_module_state_t *moduleState = UhkModuleStates + moduleId;
        if (moduleState->pointerCount) {
            if (moduleState->moduleId == ModuleId_KeyClusterLeft) {
                ActiveUsbMouseReport->wheelX += moduleState->pointerDelta.x;
                ActiveUsbMouseReport->wheelY -= moduleState->pointerDelta.y;
            } else {
                ActiveUsbMouseReport->x += moduleState->pointerDelta.x;
                ActiveUsbMouseReport->y -= moduleState->pointerDelta.y;
            }
            moduleState->pointerDelta.x = 0;
            moduleState->pointerDelta.y = 0;
        }
    }

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
    if (activeMouseStates[SerializedMouseAction_Button_4]) {
        ActiveUsbMouseReport->buttons |= MouseButton_4;
    }
    if (activeMouseStates[SerializedMouseAction_Button_5]) {
        ActiveUsbMouseReport->buttons |= MouseButton_5;
    }
    if (activeMouseStates[SerializedMouseAction_Button_6]) {
        ActiveUsbMouseReport->buttons |= MouseButton_6;
    }
    if (activeMouseStates[SerializedMouseAction_Button_7]) {
        ActiveUsbMouseReport->buttons |= MouseButton_7;
    }
    if (activeMouseStates[SerializedMouseAction_Button_8]) {
        ActiveUsbMouseReport->buttons |= MouseButton_8;
    }
}

layer_id_t PreviousLayer = LayerId_Base;

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

    if (!keyState->previous && PreviousLayer == LayerId_Base && action->switchLayer.mode == SwitchLayerMode_HoldAndDoubleTapToggle) {
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

// Sticky modifiers are all "action modifiers" - i.e., modifiers of composed
// keystrokes whose purpose is to activate concrete shortcut. They are
// activated once on keydown, and reset when another key gets activated (even
// if the activation key is still active).
//
// Depending on configuration, they may "stick" - i.e., live longer than their
// activation key, either until next action, or until release of held layer.
// (This serves for Alt+Tab style shortcuts.)
static uint8_t stickyModifiers;
static key_state_t* stickyModifierKey;
static bool    stickyModifierShouldStick;

//todo: refactor - make this part of layer handling mechanism
static uint8_t secondaryRoleLayer = LayerId_Base;
static key_state_t* secondaryRoleLayerKey;

static bool isStickyShortcut(key_action_t * action)
{
    if (action->keystroke.modifiers == 0 || action->type != KeyActionType_Keystroke || action->keystroke.keystrokeType != KeystrokeType_Basic) {
        return false;
    }

    const uint8_t alt = HID_KEYBOARD_MODIFIER_LEFTALT | HID_KEYBOARD_MODIFIER_RIGHTALT;
    const uint8_t super = HID_KEYBOARD_MODIFIER_LEFTGUI | HID_KEYBOARD_MODIFIER_RIGHTGUI;
    const uint8_t ctrl = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_RIGHTCTRL;

    switch(action->keystroke.scancode) {
        case HID_KEYBOARD_SC_TAB:
        case HID_KEYBOARD_SC_LEFT_ARROW:
        case HID_KEYBOARD_SC_RIGHT_ARROW:
        case HID_KEYBOARD_SC_UP_ARROW:
        case HID_KEYBOARD_SC_DOWN_ARROW:
            return action->keystroke.modifiers & (alt | super | ctrl);
        default:
            return false;
    }
}

static bool shouldStickAction(key_action_t * action)
{
    //todo: refactor - ideally make secondaryRoleLayer be handled by isLayerHeld
    bool currentLayerIsHeld = IsLayerHeld() || (secondaryRoleLayer != LayerId_Base );
    return currentLayerIsHeld && isStickyShortcut(action) && StickyModifiersEnabled;
}

static void activateStickyMods(key_state_t *keyState, key_action_t *action)
{
    stickyModifiers = action->keystroke.modifiers;
    stickyModifierKey = keyState;
    stickyModifierShouldStick = shouldStickAction(action);
}

static void applyKeystrokePrimary(key_state_t *keyState, key_action_t *action)
{
    if (KeyState_Active(keyState)) {
        bool stickyModifiersChanged = false;
        if (action->keystroke.scancode) {
            // On keydown, reset old sticky modifiers and set new ones
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiersChanged = action->keystroke.modifiers != stickyModifiers;
                activateStickyMods(keyState, action);
            }
        } else {
            if(!SuppressMods) {
                ActiveUsbBasicKeyboardReport->modifiers |= action->keystroke.modifiers;
            }
        }
        HardwareModifierState |= action->keystroke.modifiers;
        // If there are mods: first cycle send just mods, in next cycle start sending mods+scancode
        if (!stickyModifiersChanged || KeyState_ActivatedEarlier(keyState)) {
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
        }
    } else if (KeyState_DeactivatedNow(keyState)) {
        if (stickyModifierKey == keyState && !stickyModifierShouldStick) {
            //disable the modifiers, but send one last report of modifiers without scancode
            ActiveUsbBasicKeyboardReport->modifiers |= stickyModifiers;
            stickyModifiers = 0;
        }
    }
}

static void applyKeystrokeSecondary(key_state_t *keyState, secondary_role_t secondaryRole)
{
    if ( IS_SECONDARY_ROLE_LAYER_SWITCHER(secondaryRole) ) {
        if (KeyState_ActivatedNow(keyState)) {
            secondaryRoleLayer = SECONDARY_ROLE_LAYER_TO_LAYER_ID(secondaryRole);
            secondaryRoleLayerKey = keyState;
        } else if (KeyState_DeactivatedNow(keyState) && secondaryRoleLayerKey == keyState) {
            secondaryRoleLayer = LayerId_Base;
            secondaryRoleLayerKey = NULL;
        }
    } else if (IS_SECONDARY_ROLE_MODIFIER(secondaryRole)) {
        ActiveUsbBasicKeyboardReport->modifiers |= SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secondaryRole);
    }
}

static void applyKeystroke(key_state_t *keyState, key_action_t *action)
{
    if (action->keystroke.secondaryRole) {
        switch (SecondaryRoles_ResolveState(keyState)) {
            case SecondaryRoleState_Primary:
                applyKeystrokePrimary(keyState, action);
                return;
            case SecondaryRoleState_Secondary:
                applyKeystrokeSecondary(keyState, action->keystroke.secondaryRole);
                return;
            case SecondaryRoleState_DontKnowYet:
                // Repeatedly trigger to keep Postponer in postponing mode until the driver decides.
                PostponerCore_PostponeNCycles(1);
                return;
        }
    } else {
        applyKeystrokePrimary(keyState, action);
    }
}

static void applyKeyAction(key_state_t *keyState, key_action_t *action, uint8_t slotId, uint8_t keyId)
{
    if (KeyState_ActivatedNow(keyState)) {
        Macros_SignalInterrupt();
    }

    switch (action->type) {
        case KeyActionType_Keystroke:
            if (KeyState_NonZero(keyState)) {
                applyKeystroke(keyState, action);
            }
            break;
        case KeyActionType_Mouse:
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiers = 0;
                activateDirectionSigns(action->mouseAction);
            }
            activeMouseStates[action->mouseAction] = true;
            break;
        case KeyActionType_SwitchLayer:
            // Handled by handleSwitchLayerAction()
            break;
        case KeyActionType_SwitchKeymap:
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiers = 0;
                SwitchKeymapById(action->switchKeymap.keymapId);
                Macros_UpdateLayerStack();
            }
            break;
        case KeyActionType_PlayMacro:
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiers = 0;
                Macros_StartMacro(action->playMacro.macroId, keyState, 255);
            }
            break;
    }
}

void clearActiveReports(void)
{
    memset(ActiveUsbMouseReport, 0, sizeof *ActiveUsbMouseReport);
    memset(ActiveUsbBasicKeyboardReport, 0, sizeof *ActiveUsbBasicKeyboardReport);
    memset(ActiveUsbMediaKeyboardReport, 0, sizeof *ActiveUsbMediaKeyboardReport);
    memset(ActiveUsbSystemKeyboardReport, 0, sizeof *ActiveUsbSystemKeyboardReport);
    basicScancodeIndex = 0;
    mediaScancodeIndex = 0;
    systemScancodeIndex = 0;
}


void mergeReports(void)
{
    for(uint8_t j = 0; j < MACRO_STATE_POOL_SIZE; j++) {
        if(MacroState[j].reportsUsed) {
            //if the macro ended right now, we still want to flush the last report
            MacroState[j].reportsUsed &= MacroState[j].macroPlaying;
            macro_state_t *s = &MacroState[j];
            ActiveUsbBasicKeyboardReport->modifiers |= s->macroBasicKeyboardReport.modifiers;
            for ( int i = 0; i < USB_BASIC_KEYBOARD_MAX_KEYS && s->macroBasicKeyboardReport.scancodes[i] != 0; i++) {
                if( basicScancodeIndex < USB_BASIC_KEYBOARD_MAX_KEYS ) {
                    ActiveUsbBasicKeyboardReport->scancodes[basicScancodeIndex++] = s->macroBasicKeyboardReport.scancodes[i];
                }
            }
            for ( int i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS && s->macroMediaKeyboardReport.scancodes[i] != 0 ; i++) {
                if( mediaScancodeIndex < USB_MEDIA_KEYBOARD_MAX_KEYS ) {
                    ActiveUsbMediaKeyboardReport->scancodes[mediaScancodeIndex++] = s->macroMediaKeyboardReport.scancodes[i];
                }
            }
            for ( int i = 0; i < USB_SYSTEM_KEYBOARD_MAX_KEYS && s->macroSystemKeyboardReport.scancodes[i] != 0; i++) {
                if( systemScancodeIndex < USB_SYSTEM_KEYBOARD_MAX_KEYS ) {
                    ActiveUsbSystemKeyboardReport->scancodes[systemScancodeIndex++] = s->macroSystemKeyboardReport.scancodes[i];
                }
            }
            ActiveUsbMouseReport->buttons |= s->macroMouseReport.buttons;
            ActiveUsbMouseReport->x += s->macroMouseReport.x;
            ActiveUsbMouseReport->y += s->macroMouseReport.y;
            ActiveUsbMouseReport->wheelX += s->macroMouseReport.wheelX;
            ActiveUsbMouseReport->wheelY += s->macroMouseReport.wheelY;
        }
    }
}

static void commitKeyState(key_state_t *keyState, bool active)
{
    if (PostponerCore_IsActive()) {
        PostponerCore_TrackKeyEvent(keyState, active);
    } else {
        keyState->current = active;
    }
}

static inline void preprocessKeyState(key_state_t *keyState)
{
    uint8_t debounceTime = keyState->previous ? DebounceTimePress : DebounceTimeRelease;
    if (keyState->debouncing && (uint8_t)(CurrentTime - keyState->timestamp) > debounceTime) {
        keyState->debouncing = false;
    }

    if (!keyState->debouncing && keyState->debouncedSwitchState != keyState->hardwareSwitchState) {
        keyState->timestamp = CurrentTime;
        keyState->debouncing = true;
        keyState->debouncedSwitchState = keyState->hardwareSwitchState;

        commitKeyState(keyState, keyState->debouncedSwitchState);
    }
}

uint32_t LastUsbGetKeyboardStateRequestTimestamp;

static void updateActiveUsbReports(void)
{
    clearActiveReports();
    HardwareModifierStatePrevious = HardwareModifierState;
    HardwareModifierState = 0;
    SuppressMods = false;


    if (MacroPlaying) {
        Macros_ContinueMacro();
    }

    memset(activeMouseStates, 0, ACTIVE_MOUSE_STATES_COUNT);

    basicScancodeIndex = 0;
    mediaScancodeIndex = 0;
    systemScancodeIndex = 0;

    layer_id_t activeLayer = LayerId_Base;
    if (activeLayer == LayerId_Base) {
        activeLayer = secondaryRoleLayer;
    }
    if (activeLayer == LayerId_Base) {
        activeLayer = GetActiveLayer();
    }
    //todo: throw this out
    bool layerChanged = PreviousLayer != activeLayer;
    if (layerChanged) {
        stickyModifiers = 0;
    }
    LedDisplay_SetLayer(activeLayer);

    LedDisplay_SetIcon(LedDisplayIcon_Agent, CurrentTime - LastUsbGetKeyboardStateRequestTimestamp < 1000);

    //todo: refactor this
    if (TestUsbStack) {
        static bool simulateKeypresses, isEven, isEvenMedia;
        static uint32_t mediaCounter = 0;
        key_state_t *testKeyState = &KeyStates[SlotId_LeftKeyboardHalf][0];

        if (activeLayer == LayerId_Fn && KeyState_ActivatedNow(testKeyState)) {
            simulateKeypresses = !simulateKeypresses;
        }
        if (simulateKeypresses) {
            isEven = !isEven;
            ActiveUsbBasicKeyboardReport->scancodes[basicScancodeIndex++] = isEven ? HID_KEYBOARD_SC_A : HID_KEYBOARD_SC_BACKSPACE;
            if (++mediaCounter % 200 == 0) {
                isEvenMedia = !isEvenMedia;
                ActiveUsbMediaKeyboardReport->scancodes[mediaScancodeIndex++] = isEvenMedia ? MEDIA_VOLUME_DOWN : MEDIA_VOLUME_UP;
            }
            MouseMoveState.xOut = isEven ? -5 : 5;
        }
    }

    // This has to happen:
    // - after GetActiveLayer()
    // - before new key activations via Postponer
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];
            keyState->previous = keyState->current;
        }
    }

    if ( PostponerCore_IsActive() ) {
        PostponerCore_RunPostponedEvents();
    }

    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];
            key_action_t *action;

            preprocessKeyState(keyState);

            if (KeyState_ActivatedNow(keyState)) {
                if (SleepModeActive) {
                    WakeUpHost();
                }
                actionCache[slotId][keyId] = CurrentKeymap[activeLayer][slotId][keyId];
            }

            action = &actionCache[slotId][keyId];

            //todo: refactor this thing
            if (KeyState_Active(keyState)) {
                handleSwitchLayerAction(keyState, action);

            }

            if (KeyState_NonZero(keyState)) {
                applyKeyAction(keyState, action, slotId, keyId);
            }
        }
    }

    PostponerCore_FinishCycle();

    mergeReports();

    processMouseActions();

    // When a layer switcher key gets pressed along with another key that produces some modifiers
    // and the accomanying key gets released then keep the related modifiers active a long as the
    // layer switcher key stays pressed.  Useful for Alt+Tab keymappings and the like.
    ActiveUsbBasicKeyboardReport->modifiers |= stickyModifiers;

    PreviousLayer = activeLayer;
}

uint32_t UsbReportUpdateCounter;

void UpdateUsbReports(void)
{
    static uint32_t lastUpdateTime;

    for (uint8_t keyId = 0; keyId < RIGHT_KEY_MATRIX_KEY_COUNT; keyId++) {
        KeyStates[SlotId_RightKeyboardHalf][keyId].hardwareSwitchState = RightKeyMatrix.keyStates[keyId];
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
        MacroRecorder_RecordBasicReport(ActiveUsbBasicKeyboardReport);

        if(RuntimeMacroRecordingBlind) {
            //just switch reports without sending the report
            ActiveUsbBasicKeyboardReport = GetInactiveUsbBasicKeyboardReport();
		} else {
            UsbReportUpdateSemaphore |= 1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX;
            usb_status_t status = UsbBasicKeyboardAction();
            //The semaphore has to be set before the call. Assume what happens if a bus reset happens asynchronously here. (Deadlock.)
            if (status != kStatus_USB_Success) {
                //This is *not* asynchronously safe as long as multiple reports of different type can be sent at the same time.
                //TODO: consider either making it atomic, or lowering semaphore reset delay, or changing subsequent ifs to elseifs
                UsbReportUpdateSemaphore &= ~(1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX);
            }
        }
    }

    if (HasUsbMediaKeyboardReportChanged) {
        UsbReportUpdateSemaphore |= 1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX;
        usb_status_t status = UsbMediaKeyboardAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX);
        }
    }

    if (HasUsbSystemKeyboardReportChanged) {
        UsbReportUpdateSemaphore |= 1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX;
        usb_status_t status = UsbSystemKeyboardAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX);
        }
    }

    // Send out the mouse position and wheel values continuously if the report is not zeros, but only send the mouse button states when they change.
    if (HasUsbMouseReportChanged || ActiveUsbMouseReport->x || ActiveUsbMouseReport->y ||
            ActiveUsbMouseReport->wheelX || ActiveUsbMouseReport->wheelY) {
        UsbReportUpdateSemaphore |= 1 << USB_MOUSE_INTERFACE_INDEX;
        usb_status_t status = UsbMouseAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_MOUSE_INTERFACE_INDEX);
        }
    }
}
