#include <math.h>
#include "key_action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "keymap.h"
#include "peripherals/test_led.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "macros.h"
#include "key_states.h"
#include "right_key_matrix.h"
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
#include "layer_switcher.h"
#include "mouse_controller.h"
#include "debug.h"

bool TestUsbStack = false;
static key_action_cached_t actionCache[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

volatile uint8_t UsbReportUpdateSemaphore = 0;

// Modifiers can be applied as one of the following classes
// - input:
//   - affect "ifMod" (in next cycle)
//   - can be suppressed
//   - by default will be outputted into the report, unless supressed
// - output:
//   - do not affect "ifMod" conditions
//   - will always be outputted into the report
// - sticky mods:
//   - these are stateful and change state so that modifiers of last pressed
//     composite shortcut is applied

uint8_t InputModifiers;
uint8_t InputModifiersPrevious;
uint8_t OutputModifiers;
bool SuppressMods = false;
bool SuppressKeys = false;
sticky_strategy_t StickyModifierStrategy = Stick_Smart;

uint16_t KeystrokeDelay = 0;

key_state_t* EmergencyKey = NULL;

// Holds are applied on current base layer.
static void applyLayerHolds(key_state_t *keyState, key_action_t *action) {
    if (action->type == KeyActionType_SwitchLayer && KeyState_Active(keyState)) {
        switch(action->switchLayer.mode) {
            case SwitchLayerMode_HoldAndDoubleTapToggle:
            case SwitchLayerMode_Hold:
                LayerSwitcher_HoldLayer(action->switchLayer.layer, false);
                break;
            case SwitchLayerMode_Toggle:
                //this switch handles only "hold" effects, therefore toggle not present here.
                break;
        }
    }

    if (
            ActiveLayer != LayerId_Base &&
            action->type == KeyActionType_Keystroke &&
            action->keystroke.secondaryRole &&
            IS_SECONDARY_ROLE_LAYER_SWITCHER(action->keystroke.secondaryRole) &&
            KeyState_Active(keyState)
    ) {
        // If some layer is active, always assume base secondary layer switcher roles to take their secondary role and be active
        // This makes secondary layer holds act just as standard layer holds.
        // Also, this is a no-op until some other event causes deactivation of the currently active
        // layer - then this layer switcher becomes active due to hold semantics.
        LayerSwitcher_HoldLayer(SECONDARY_ROLE_LAYER_TO_LAYER_ID(action->keystroke.secondaryRole), false);
    }
}

// Toggle actions are applied on active/cached layer.
static void applyToggleLayerAction(key_state_t *keyState, key_action_t *action) {
    switch(action->switchLayer.mode) {
        case SwitchLayerMode_HoldAndDoubleTapToggle:
            if( keyState->current != keyState->previous ) {
                LayerSwitcher_DoubleTapToggle(action->switchLayer.layer, keyState);
            }
            break;
        case SwitchLayerMode_Toggle:
            if (KeyState_ActivatedNow(keyState)) {
                LayerSwitcher_ToggleLayer(action->switchLayer.layer);
            }
            break;
        case SwitchLayerMode_Hold:
            if (KeyState_ActivatedNow(keyState)) {
                LayerSwitcher_UnToggleLayerOnly(action->switchLayer.layer);
            }
            break;
    }
}

static void handleEventInterrupts(key_state_t *keyState) {
    if(KeyState_ActivatedNow(keyState)) {
        LayerSwitcher_DoubleTapInterrupt(keyState);
    }
}

// Sticky modifiers are all "action modifiers" - i.e., modifiers of composed
// keystrokes whose purpose is to activate specific shortcut. They are
// activated once on keydown, and reset when another key gets activated (even
// if the activation key is still active).
//
// Depending on configuration, they may "stick" - i.e., live longer than their
// activation key, either until next action, or until release of held layer.
// (This serves for Alt+Tab style shortcuts.)
static uint8_t stickyModifiersNegative;
static uint8_t stickyModifiers;
static key_state_t* stickyModifierKey;
static bool    stickyModifierShouldStick;


static bool isStickyShortcut(key_action_t * action)
{
    if (action->keystroke.modifiers == 0 || action->type != KeyActionType_Keystroke || action->keystroke.keystrokeType != KeystrokeType_Basic) {
        return false;
    }

    const uint8_t alt = HID_KEYBOARD_MODIFIER_LEFTALT | HID_KEYBOARD_MODIFIER_RIGHTALT;
    const uint8_t super = HID_KEYBOARD_MODIFIER_LEFTGUI | HID_KEYBOARD_MODIFIER_RIGHTGUI;
    const uint8_t ctrl = HID_KEYBOARD_MODIFIER_LEFTCTRL | HID_KEYBOARD_MODIFIER_RIGHTCTRL;

    switch(action->keystroke.scancode) {
        case HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE:
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
    switch(StickyModifierStrategy) {
    case Stick_Always:
        return true;
    case Stick_Never:
        return false;
    default:
    case Stick_Smart:
        return ActiveLayerHeld && isStickyShortcut(action);
    }
}

static void activateStickyMods(key_state_t *keyState, key_action_cached_t *action)
{
    stickyModifiersNegative = action->modifierLayerMask;
    stickyModifiers = action->action.keystroke.modifiers;
    stickyModifierKey = keyState;
    stickyModifierShouldStick = shouldStickAction(&action->action);
}

void ActivateStickyMods(key_state_t *keyState, uint8_t mods)
{
    stickyModifiersNegative = 0;
    stickyModifiers = mods;
    stickyModifierKey = keyState;
    stickyModifierShouldStick = true;
}

static void applyKeystrokePrimary(key_state_t *keyState, key_action_cached_t *cachedAction)
{
    if (KeyState_Active(keyState)) {
        key_action_t* action = &cachedAction->action;
        bool stickyModifiersChanged = false;
        if (action->keystroke.scancode) {
            // On keydown, reset old sticky modifiers and set new ones
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiersChanged |= action->keystroke.modifiers != stickyModifiers;
                stickyModifiersChanged |= cachedAction->modifierLayerMask != stickyModifiersNegative;
                activateStickyMods(keyState, cachedAction);
            }
        } else {
            InputModifiers |= action->keystroke.modifiers;
        }
        // If there are mods: first cycle send just mods, in next cycle start sending mods+scancode
        if (!stickyModifiersChanged || KeyState_ActivatedEarlier(keyState)) {
            switch (action->keystroke.keystrokeType) {
                case KeystrokeType_Basic:
                    UsbBasicKeyboard_AddScancode(ActiveUsbBasicKeyboardReport, action->keystroke.scancode);
                    break;
                case KeystrokeType_Media:
                    UsbMediaKeyboard_AddScancode(ActiveUsbMediaKeyboardReport, action->keystroke.scancode);
                    break;
                case KeystrokeType_System:
                    UsbSystemKeyboard_AddScancode(ActiveUsbSystemKeyboardReport, action->keystroke.scancode);
                    break;
            }
        }
    } else if (KeyState_DeactivatedNow(keyState)) {
        if (stickyModifierKey == keyState && !stickyModifierShouldStick) {
            //disable the modifiers, but send one last report of modifiers without scancode
            OutputModifiers |= stickyModifiers;
            stickyModifiers = 0;
            stickyModifiersNegative = 0;
        }
    }
}

static void applyKeystrokeSecondary(key_state_t *keyState, key_action_t *action, key_action_t *actionBase)
{
    secondary_role_t secondaryRole = action->keystroke.secondaryRole;
    if ( IS_SECONDARY_ROLE_LAYER_SWITCHER(secondaryRole) ) {
        // If the cached action is the current base role, then hold, otherwise keymap was changed. In that case do nothing just
        // as a well behaved hold action should.
        if(action->type == actionBase->type && action->keystroke.secondaryRole == actionBase->keystroke.secondaryRole) {
            LayerSwitcher_HoldLayer(SECONDARY_ROLE_LAYER_TO_LAYER_ID(secondaryRole), false);
        }
    } else if (IS_SECONDARY_ROLE_MODIFIER(secondaryRole)) {
        InputModifiers |= SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secondaryRole);
    }
}

static void applyKeystroke(key_state_t *keyState, key_action_cached_t *cachedAction, key_action_t *actionBase)
{
    key_action_t* action = &cachedAction->action;
    if (action->keystroke.secondaryRole) {
        switch (SecondaryRoles_ResolveState(keyState, action->keystroke.secondaryRole)) {
            case SecondaryRoleState_Primary:
                applyKeystrokePrimary(keyState, cachedAction);
                return;
            case SecondaryRoleState_Secondary:
                applyKeystrokeSecondary(keyState, action, actionBase);
                return;
            case SecondaryRoleState_DontKnowYet:
                // Repeatedly trigger to keep Postponer in postponing mode until the driver decides.
                PostponerCore_PostponeNCycles(1);
                return;
        }
    } else {
        applyKeystrokePrimary(keyState, cachedAction);
    }
}

void ApplyKeyAction(key_state_t *keyState, key_action_cached_t *cachedAction, key_action_t *actionBase)
{
    key_action_t* action = &cachedAction->action;

    if (KeyState_ActivatedNow(keyState)) {
        Macros_SignalInterrupt();
    }

    switch (action->type) {
        case KeyActionType_Keystroke:
            if (KeyState_NonZero(keyState)) {
                applyKeystroke(keyState, cachedAction, actionBase);
            }
            break;
        case KeyActionType_Mouse:
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiers = 0;
                stickyModifiersNegative = 0;
                MouseController_ActivateDirectionSigns(action->mouseAction);
            }
            ActiveMouseStates[action->mouseAction]++;
            break;
        case KeyActionType_SwitchLayer:
            if (keyState->current != keyState->previous) {
                applyToggleLayerAction(keyState, action);
            }
            break;
        case KeyActionType_SwitchKeymap:
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiers = 0;
                stickyModifiersNegative = 0;
                SwitchKeymapById(action->switchKeymap.keymapId);
                Macros_ResetLayerStack();
            }
            break;
        case KeyActionType_PlayMacro:
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiers = 0;
                stickyModifiersNegative = 0;
                Macros_StartMacro(action->playMacro.macroId, keyState, 255, true);
            }
            break;
    }
}

static void clearActiveReports(void)
{
    memset(ActiveUsbMouseReport, 0, sizeof *ActiveUsbMouseReport);
    memset(ActiveUsbBasicKeyboardReport, 0, sizeof *ActiveUsbBasicKeyboardReport);
    memset(ActiveUsbMediaKeyboardReport, 0, sizeof *ActiveUsbMediaKeyboardReport);
    memset(ActiveUsbSystemKeyboardReport, 0, sizeof *ActiveUsbSystemKeyboardReport);
}


static void mergeReports(void)
{
    for(uint8_t j = 0; j < MACRO_STATE_POOL_SIZE; j++) {
        if(MacroState[j].ms.reportsUsed) {
            //if the macro ended right now, we still want to flush the last report
            MacroState[j].ms.reportsUsed &= MacroState[j].ms.macroPlaying;
            macro_state_t *s = &MacroState[j];

            UsbBasicKeyboard_MergeReports(&(s->ms.macroBasicKeyboardReport), ActiveUsbBasicKeyboardReport);
            UsbMediaKeyboard_MergeReports(&(s->ms.macroMediaKeyboardReport), ActiveUsbMediaKeyboardReport);
            UsbSystemKeyboard_MergeReports(&(s->ms.macroSystemKeyboardReport), ActiveUsbSystemKeyboardReport);

            InputModifiers |= s->ms.inputModifierMask;

            ActiveUsbMouseReport->buttons |= s->ms.macroMouseReport.buttons;
            ActiveUsbMouseReport->x += s->ms.macroMouseReport.x;
            ActiveUsbMouseReport->y += s->ms.macroMouseReport.y;
            ActiveUsbMouseReport->wheelX += s->ms.macroMouseReport.wheelX;
            ActiveUsbMouseReport->wheelY += s->ms.macroMouseReport.wheelY;
        }
    }
}

static void commitKeyState(key_state_t *keyState, bool active)
{
    WATCH_TRIGGER(keyState);
    if (PostponerCore_IsActive()) {
        PostponerCore_TrackKeyEvent(keyState, active, 255);
    } else {
        keyState->current = active;
    }
    WAKE_MACROS_ON_KEYSTATE_CHANGE();
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

static void handleUsbStackTestMode() {
    if (TestUsbStack) {
        static bool simulateKeypresses, isEven, isEvenMedia;
        static uint32_t mediaCounter = 0;
        key_state_t *testKeyState = &KeyStates[SlotId_LeftKeyboardHalf][0];

        if (ActiveLayer == LayerId_Fn && testKeyState->current && !testKeyState->previous) {
            simulateKeypresses = !simulateKeypresses;
        }
        if (simulateKeypresses) {
            isEven = !isEven;
            UsbBasicKeyboard_AddScancode(ActiveUsbBasicKeyboardReport,
                    isEven ? HID_KEYBOARD_SC_A : HID_KEYBOARD_SC_BACKSPACE);
            if (++mediaCounter % 200 == 0) {
                isEvenMedia = !isEvenMedia;
                UsbMediaKeyboard_AddScancode(ActiveUsbMediaKeyboardReport,
                        isEvenMedia ? MEDIA_VOLUME_DOWN : MEDIA_VOLUME_UP);
            }
            MouseMoveState.xOut = isEven ? -5 : 5;
        }
    }
}

//This might be moved directly into the layer_switcher
static void handleLayerChanges() {
    static layer_id_t previousLayer = LayerId_Base;

    LayerSwitcher_UpdateActiveLayer();

    if(ActiveLayer != previousLayer) {
        previousLayer = ActiveLayer;
        stickyModifiers = 0;
        stickyModifiersNegative = 0;
    }
}

static void updateActiveUsbReports(void)
{
    clearActiveReports();
    InputModifiersPrevious = InputModifiers;
    InputModifiers = 0;
    OutputModifiers = 0;
    SuppressMods = false;

    if (MacroPlaying) {
        if (Macros_WakeMeOnTime < CurrentTime) {
            Macros_WakedBecauseOfTime = true;
            MacroPlaying = true;
        }
        Macros_ContinueMacro();
    }

    memcpy(ActiveMouseStates, ToggledMouseStates, ACTIVE_MOUSE_STATES_COUNT);

    handleLayerChanges();

    if ( CurrentTime - LastUsbGetKeyboardStateRequestTimestamp < 2000) {
       LedDisplay_SetIcon(LedDisplayIcon_Agent, CurrentTime - LastUsbGetKeyboardStateRequestTimestamp < 1000);
    }

    handleUsbStackTestMode();

    if (PostponerCore_IsActive()) {
        PostponerCore_RunPostponedEvents();
    }

    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];
            key_action_cached_t *cachedAction;
            key_action_t *actionBase;

            if(((uint8_t*)keyState)[1] == 0) {
                continue;
            }

            preprocessKeyState(keyState);

            if (KeyState_NonZero(keyState)) {
                if (KeyState_ActivatedNow(keyState)) {
                    // cache action so that key's meaning remains the same as long
                    // as it is pressed
                    actionCache[slotId][keyId].modifierLayerMask = 0;
                    if (SleepModeActive) {
                        WakeUpHost();
                    }
                    if (Postponer_LastKeyLayer != 255 && PostponerCore_IsActive()) {
                        actionCache[slotId][keyId].action = CurrentKeymap[Postponer_LastKeyLayer][slotId][keyId];
                        Postponer_LastKeyLayer = 255;
                    } else if (LayerConfig[ActiveLayer].modifierLayerMask != 0) {
                        if (CurrentKeymap[ActiveLayer][slotId][keyId].type != KeyActionType_None) {
                            actionCache[slotId][keyId].action = CurrentKeymap[ActiveLayer][slotId][keyId];
                            actionCache[slotId][keyId].modifierLayerMask = ActiveLayerModifierMask;
                        } else {
                            actionCache[slotId][keyId].action = CurrentKeymap[LayerId_Base][slotId][keyId];
                        }
                    } else {
                        actionCache[slotId][keyId].action = CurrentKeymap[ActiveLayer][slotId][keyId];
                    }
                    handleEventInterrupts(keyState);
                }

                cachedAction = &actionCache[slotId][keyId];
                actionBase = &CurrentKeymap[LayerId_Base][slotId][keyId];

                //apply base-layer holds
                applyLayerHolds(keyState, actionBase);

                //apply active-layer action
                ApplyKeyAction(keyState, cachedAction, actionBase);

                keyState->previous = keyState->current;
            }
        }
    }

    MouseController_ProcessMouseActions();

    PostponerCore_FinishCycle();

    mergeReports();

    // When a layer switcher key gets pressed along with another key that produces some modifiers
    // and the accomanying key gets released then keep the related modifiers active a long as the
    // layer switcher key stays pressed.  Useful for Alt+Tab keymappings and the like.

    uint8_t maskedInputMods = (~stickyModifiersNegative) & InputModifiers;
    ActiveUsbBasicKeyboardReport->modifiers |= SuppressMods ? 0 : maskedInputMods;
    ActiveUsbBasicKeyboardReport->modifiers |= OutputModifiers | stickyModifiers;
}

void justPreprocessInput(void) {
    // Make preprocessKeyState push new events into postponer queue.
    // As a side-effect, postpone first cycle after we switch back to regular update loop
    PostponerCore_PostponeNCycles(0);
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];

            preprocessKeyState(keyState);
        }
    }
}

uint32_t UsbReportUpdateCounter;

static void updateLedSleepModeState(uint32_t lastActivityTime) {
    uint32_t elapsedTime = Timer_GetElapsedTime(&lastActivityTime);

    if (elapsedTime > LedSleepTimeout && !LedSleepModeActive && LedSleepTimeout) {
        LedSleepModeActive = true;
        LedSlaveDriver_UpdateLeds();
    } else if (elapsedTime < LedSleepTimeout && LedSleepModeActive) {
        LedSleepModeActive = false;
        LedSlaveDriver_UpdateLeds();
    }
}

void UpdateUsbReports(void)
{
    static uint32_t lastUpdateTime;
    static uint32_t lastReportTime;
    static uint32_t lastActivityTime;

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

    if (Timer_GetElapsedTime(&lastReportTime) < KeystrokeDelay) {
        justPreprocessInput();
        return;
    }

    lastUpdateTime = CurrentTime;
    UsbReportUpdateCounter++;

    UsbBasicKeyboardResetActiveReport();
    UsbMediaKeyboardResetActiveReport();
    UsbSystemKeyboardResetActiveReport();
    UsbMouseResetActiveReport();

    updateActiveUsbReports();

    updateLedSleepModeState(lastActivityTime);

    if (UsbBasicKeyboardCheckReportReady() == kStatus_USB_Success) {
        MacroRecorder_RecordBasicReport(ActiveUsbBasicKeyboardReport);

        if(RuntimeMacroRecordingBlind) {
            //just switch reports without sending the report
            UsbBasicKeyboardResetActiveReport();
		} else {
            UsbReportUpdateSemaphore |= 1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX;
            usb_status_t status = UsbBasicKeyboardAction();
            //The semaphore has to be set before the call. Assume what happens if a bus reset happens asynchronously here. (Deadlock.)
            if (status != kStatus_USB_Success) {
                //This is *not* asynchronously safe as long as multiple reports of different type can be sent at the same time.
                //TODO: consider either making it atomic, or lowering semaphore reset delay
                UsbReportUpdateSemaphore &= ~(1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX);
            }
        }
        lastReportTime = CurrentTime;
        lastActivityTime = CurrentTime;
    }

    if (UsbMediaKeyboardCheckReportReady() == kStatus_USB_Success) {
        UsbReportUpdateSemaphore |= 1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX;
        usb_status_t status = UsbMediaKeyboardAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX);
        }
        lastActivityTime = CurrentTime;
    }

    if (UsbSystemKeyboardCheckReportReady() == kStatus_USB_Success) {
        UsbReportUpdateSemaphore |= 1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX;
        usb_status_t status = UsbSystemKeyboardAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX);
        }
        lastActivityTime = CurrentTime;
    }

    if (UsbMouseCheckReportReady() == kStatus_USB_Success) {
        UsbReportUpdateSemaphore |= 1 << USB_MOUSE_INTERFACE_INDEX;
        usb_status_t status = UsbMouseAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_MOUSE_INTERFACE_INDEX);
        }
        lastActivityTime = CurrentTime;
    }
}
