#include <math.h>
#include "atomicity.h"
#include "event_scheduler.h"
#include "key_action.h"
#include "led_display.h"
#include "layer.h"
#include "ledmap.h"
#include "stubs.h"
#include "test_switches.h"
#include "slot.h"
#include "keymap.h"
#include "key_matrix.h"
#ifndef __ZEPHYR__
#include "peripherals/test_led.h"
#include "right_key_matrix.h"
#include "slave_drivers/touchpad_driver.h"
#endif
#include "slave_drivers/is31fl3xxx_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "key_states.h"
#include "usb_report_updater.h"
#include "timer.h"
#include "config_parser/parse_keymap.h"
#include "usb_commands/usb_command_get_debug_buffer.h"
#include "arduino_hid/ConsumerAPI.h"
#include "macro_recorder.h"
#include "macros/shortcut_parser.h"
#include "postponer.h"
#include "secondary_role_driver.h"
#include "layer_switcher.h"
#include "layer_stack.h"
#include "mouse_controller.h"
#include "mouse_keys.h"
#include "utils.h"
#include "debug.h"
#include "macros/key_timing.h"
#include "config_manager.h"
#include <string.h>
#include "led_manager.h"
#include "power_mode.h"

#ifdef __ZEPHYR__
#include "debug_eventloop_timing.h"
#include "shell.h"
#include "usb/usb_compatibility.h"
#include "keyboard/input_interceptor.h"
#include "keyboard/charger.h"
#include "logger.h"
#else
#include "stubs.h"
#endif

bool TestUsbStack = false;

static key_action_cached_t actionCache[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

uint32_t UsbReportUpdater_LastActivityTime;

volatile uint8_t UsbReportUpdateSemaphore = 0;

// Modifiers can be applied as one of the following classes
// - input:
//   - affect "ifMod" (in next cycle)
//   - can be suppressed
//   - by default will be outputted into the report, unless supressed
//   - implementation: they are kept separately in variables that are local to report modules
// - output:
//   - do not affect "ifMod" conditions
//   - will always be outputted into the report
//   - implementation: they go straight into the associated reports and are not handled explicitly
// - sticky mods:
//   - these are stateful and change state so that modifiers of last pressed
//     composite shortcut is applied
//   - implementation: they are kept in one global place

uint8_t NativeActionInputModifiers;
uint8_t InputModifiers;
uint8_t InputModifiersPrevious;
uint8_t OutputModifiers;
bool SuppressMods = false;
bool SuppressKeys = false;

usb_keyboard_reports_t NativeKeyboardReports = {
    .recomputeStateVectorMask = EventVector_NativeActions,
    .reportsUsedVectorMask = EventVector_NativeActionReportsUsed,
    .postponeMask = EventVector_NativeActionsPostponing,
};

static void resetActiveReports() {
    memset(ActiveUsbMouseReport, 0, sizeof *ActiveUsbMouseReport);
    memset(ActiveUsbBasicKeyboardReport, 0, sizeof *ActiveUsbBasicKeyboardReport);
    memset(ActiveUsbMediaKeyboardReport, 0, sizeof *ActiveUsbMediaKeyboardReport);
    memset(ActiveUsbSystemKeyboardReport, 0, sizeof *ActiveUsbSystemKeyboardReport);
}

void UsbReportUpdater_ResetKeyboardReports(usb_keyboard_reports_t* reports) {
    memset(&reports->basic, 0, sizeof reports->basic);
    memset(&reports->media, 0, sizeof reports->media);
    memset(&reports->system, 0, sizeof reports->system);
    reports->inputModifiers = 0;
}

// Holds are applied on current base layer.
static void applyLayerHolds(key_state_t *keyState, key_action_t *action) {
    if (action->type == KeyActionType_SwitchLayer) {
        switch(action->switchLayer.mode) {
            case SwitchLayerMode_HoldAndDoubleTapToggle:
            case SwitchLayerMode_Hold:
                if (KeyState_Active(keyState)) {
                    LayerSwitcher_HoldLayer(action->switchLayer.layer, false);
                }
                if (KeyState_ActivatedNow(keyState) || KeyState_DeactivatedNow(keyState)) {
                    EventVector_Set(EventVector_LayerHolds);
                }
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
            IS_SECONDARY_ROLE_LAYER_SWITCHER(action->keystroke.secondaryRole)
    ) {
        // If some layer is active, always assume base secondary layer switcher roles to take their secondary role and be active
        // This makes secondary layer holds act just as standard layer holds.
        // Also, this is a no-op until some other event causes deactivation of the currently active
        // layer - then this layer switcher becomes active due to hold semantics.
        if ( KeyState_Active(keyState) ) {
            LayerSwitcher_HoldLayer(SECONDARY_ROLE_LAYER_TO_LAYER_ID(action->keystroke.secondaryRole), false);
        }
        if (KeyState_ActivatedNow(keyState) || KeyState_DeactivatedNow(keyState)) {
            EventVector_Set(EventVector_LayerHolds);
        }
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
        Macros_SignalInterrupt();
        UsbReportUpdater_LastActivityTime = CurrentTime;
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
uint8_t StickyModifiers;
uint8_t StickyModifiersNegative;
static key_state_t* stickyModifierKey;
static bool stickyModifierShouldStick;

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
    switch(Cfg.StickyModifierStrategy) {
    case Stick_Always:
        return true;
    case Stick_Never:
        return false;
    default:
    case Stick_Smart:
        return ActiveLayerHeld && isStickyShortcut(action);
    }
}

static void resetStickyMods(key_action_cached_t *cachedAction)
{
    StickyModifiers = 0;
    StickyModifiersNegative = cachedAction->modifierLayerMask;
    EventVector_Set(EventVector_SendUsbReports);
}

static void activateStickyMods(key_state_t *keyState, key_action_cached_t *action)
{
    StickyModifiersNegative = action->modifierLayerMask;
    StickyModifiers = action->action.keystroke.modifiers;
    stickyModifierKey = keyState;
    stickyModifierShouldStick = shouldStickAction(&action->action);
    EventVector_Set(EventVector_SendUsbReports);
}

void ActivateStickyMods(key_state_t *keyState, uint8_t mods)
{
    //do nothing to stickyModifiersNegative
    StickyModifiers = mods;
    stickyModifierKey = keyState;
    stickyModifierShouldStick = ActiveLayerHeld;
    EventVector_Set(EventVector_SendUsbReports);
}

static void applyKeystrokePrimary(key_state_t *keyState, key_action_cached_t *cachedAction, usb_keyboard_reports_t* reports)
{
    EventVector_Set(EventVector_SendUsbReports);

    if (KeyState_Active(keyState)) {
        key_action_t* action = &cachedAction->action;
        bool stickyModifiersChanged = false;
        if (action->keystroke.scancode) {
            // On keydown, reset old sticky modifiers and set new ones
            if (KeyState_ActivatedNow(keyState)) {
                stickyModifiersChanged |= action->keystroke.modifiers != StickyModifiers;
                stickyModifiersChanged |= cachedAction->modifierLayerMask != StickyModifiersNegative;
                activateStickyMods(keyState, cachedAction);
            }
        } else {
            if (action->keystroke.modifiers) {
                reports->inputModifiers |= action->keystroke.modifiers;
                EventVector_Set(reports->reportsUsedVectorMask);
            }
        }
        // If there are mods: first cycle send just mods, in next cycle start sending mods+scancode
        if (!stickyModifiersChanged || KeyState_ActivatedEarlier(keyState)) {
            switch (action->keystroke.keystrokeType) {
                case KeystrokeType_Basic:
                    UsbBasicKeyboard_AddScancode(&reports->basic, action->keystroke.scancode);
                    break;
                case KeystrokeType_Media:
                    UsbMediaKeyboard_AddScancode(&reports->media, action->keystroke.scancode);
                    break;
                case KeystrokeType_System:
                    UsbSystemKeyboard_AddScancode(&reports->system, action->keystroke.scancode);
                    break;
            }
        }
        if (stickyModifiersChanged) {
            EventVector_Set(reports->recomputeStateVectorMask | reports->postponeMask); // trigger next update in order to clear them, usually EventVector_NativeActions here
        }
    } else if (KeyState_DeactivatedNow(keyState)) {
        bool stickyModsAreNonZero = StickyModifiers != 0 || StickyModifiersNegative != 0;
        if (stickyModifierKey == keyState && !stickyModifierShouldStick && stickyModsAreNonZero) {
            //disable the modifiers, but send one last report of modifiers without scancode
            reports->basic.modifiers |= StickyModifiers;
            StickyModifiers = 0;
            StickyModifiersNegative = 0;
            EventVector_Set(
                    reports->recomputeStateVectorMask | // trigger next update in order to clear them, usually EventVector_NativeActions here
                    reports->reportsUsedVectorMask |
                    reports->postponeMask
                );
        }
    }
}

static void applyKeystrokeSecondary(key_state_t *keyState, key_action_t *action, key_action_t *actionBase, usb_keyboard_reports_t* reports)
{
    secondary_role_t secondaryRole = action->keystroke.secondaryRole;
    if ( IS_SECONDARY_ROLE_LAYER_SWITCHER(secondaryRole) ) {
        // If the cached action is the current base role, then hold, otherwise keymap was changed. In that case do nothing just
        // as a well behaved hold action should.
        if(KeyState_Active(keyState) && action->type == actionBase->type && action->keystroke.secondaryRole == actionBase->keystroke.secondaryRole) {
            LayerSwitcher_HoldLayer(SECONDARY_ROLE_LAYER_TO_LAYER_ID(secondaryRole), false);
        }
        if (KeyState_ActivatedNow(keyState) || KeyState_DeactivatedNow(keyState)) {
            EventVector_Set(EventVector_LayerHolds);
        }
    } else if (IS_SECONDARY_ROLE_MODIFIER(secondaryRole)) {
        if (KeyState_Active(keyState)) {
            reports->inputModifiers |= SECONDARY_ROLE_MODIFIER_TO_HID_MODIFIER(secondaryRole);
            EventVector_Set(EventVector_SendUsbReports | reports->reportsUsedVectorMask);
        }
    }
}

static void applyKeystroke(key_state_t *keyState, key_action_cached_t *cachedAction, key_action_t *actionBase, usb_keyboard_reports_t* reports)
{
    key_action_t* action = &cachedAction->action;
    if (action->keystroke.secondaryRole) {
        secondary_role_result_t res = SecondaryRoles_ResolveState(keyState, action->keystroke.secondaryRole, Cfg.SecondaryRoles_Strategy, KeyState_ActivatedNow(keyState), false);
        if (res.activatedNow) {
            SecondaryRoles_FakeActivation(res);
        }
        switch (res.state) {
            case SecondaryRoleState_Primary:
                applyKeystrokePrimary(keyState, cachedAction, reports);
                return;
            case SecondaryRoleState_Secondary:
                applyKeystrokeSecondary(keyState, action, actionBase, reports);
                return;
            case SecondaryRoleState_DontKnowYet:
                // Repeatedly trigger to keep Postponer in postponing mode until the driver decides.
                EventVector_Set(EventVector_NativeActionsPostponing);
                return;
        }
    } else {
        applyKeystrokePrimary(keyState, cachedAction, reports);
    }
}

void ApplyKeyAction(key_state_t *keyState, key_action_cached_t *cachedAction, key_action_t *actionBase, usb_keyboard_reports_t* reports)
{
    key_action_t* action = &cachedAction->action;

    switch (action->type) {
        case KeyActionType_Keystroke:
            if (KeyState_NonZero(keyState)) {
                EventVector_Set(EventVector_NativeActionReportsUsed);
                applyKeystroke(keyState, cachedAction, actionBase, reports);
            }
            break;
        case KeyActionType_Mouse:
            if (KeyState_ActivatedNow(keyState)) {
                resetStickyMods(cachedAction);
                MouseKeys_SetState(action->mouseAction, false, true);
            }
            if (KeyState_Active(keyState)) {
                ActiveMouseStates[action->mouseAction]++;
            }
            if (KeyState_DeactivatedNow(keyState)) {
                MouseKeys_SetState(action->mouseAction, false, false);
            }
            break;
        case KeyActionType_SwitchLayer:
            if (keyState->current != keyState->previous) {
                applyToggleLayerAction(keyState, action);
            }
            break;
        case KeyActionType_SwitchKeymap:
            if (KeyState_ActivatedNow(keyState)) {
                resetStickyMods(cachedAction);
                SwitchKeymapById(action->switchKeymap.keymapId);
                LayerStack_Reset();
            }
            break;
        case KeyActionType_PlayMacro:
            if (KeyState_ActivatedNow(keyState)) {
                resetStickyMods(cachedAction);
                Macros_StartMacro(action->playMacro.macroId, keyState, 255, true);
            }
            break;
    }
}

static void mergeReports(void)
{
    resetActiveReports();

    InputModifiers = 0;

    if (EventVector_IsSet(EventVector_MacroReportsUsed)) {
        for(uint8_t j = 0; j < MACRO_STATE_POOL_SIZE; j++) {
            if(MacroState[j].ms.reportsUsed) {
                //if the macro ended right now, we still want to flush the last report
                MacroState[j].ms.reportsUsed &= MacroState[j].ms.macroPlaying;
                macro_state_t *macroState = &MacroState[j];

                UsbBasicKeyboard_MergeReports(&(macroState->ms.macroBasicKeyboardReport), ActiveUsbBasicKeyboardReport);
                UsbMediaKeyboard_MergeReports(&(macroState->ms.macroMediaKeyboardReport), ActiveUsbMediaKeyboardReport);
                UsbSystemKeyboard_MergeReports(&(macroState->ms.macroSystemKeyboardReport), ActiveUsbSystemKeyboardReport);
                UsbMouse_MergeReports(&(macroState->ms.macroMouseReport), ActiveUsbMouseReport);

                InputModifiers |= macroState->ms.inputModifierMask;
            }
        }
    }

    if (EventVector_IsSet(EventVector_NativeActionReportsUsed)) {
        UsbBasicKeyboard_MergeReports(&NativeKeyboardReports.basic, ActiveUsbBasicKeyboardReport);
        UsbMediaKeyboard_MergeReports(&NativeKeyboardReports.media, ActiveUsbMediaKeyboardReport);
        UsbSystemKeyboard_MergeReports(&NativeKeyboardReports.system, ActiveUsbSystemKeyboardReport);
        InputModifiers |= NativeKeyboardReports.inputModifiers;
    }

    if (EventVector_IsSet(EventVector_MouseKeysReportsUsed)) {
        UsbMouse_MergeReports(&MouseKeysMouseReport, ActiveUsbMouseReport);
    }

    if (EventVector_IsSet(EventVector_MouseControllerMouseReportsUsed)) {
        UsbMouse_MergeReports(&MouseControllerMouseReport, ActiveUsbMouseReport);
    }

    if (EventVector_IsSet(EventVector_MouseControllerKeyboardReportsUsed)) {
        UsbBasicKeyboard_MergeReports(&MouseControllerKeyboardReports.basic, ActiveUsbBasicKeyboardReport);
        UsbMediaKeyboard_MergeReports(&MouseControllerKeyboardReports.media, ActiveUsbMediaKeyboardReport);
        UsbSystemKeyboard_MergeReports(&MouseControllerKeyboardReports.system, ActiveUsbSystemKeyboardReport);
    }

    // When a layer switcher key gets pressed along with another key that produces some modifiers
    // and the accomanying key gets released then keep the related modifiers active a long as the
    // layer switcher key stays pressed.  Useful for Alt+Tab keymappings and the like.

    uint8_t maskedInputMods = (~StickyModifiersNegative) & InputModifiers;
    ActiveUsbBasicKeyboardReport->modifiers |= SuppressMods ? 0 : maskedInputMods;
    ActiveUsbBasicKeyboardReport->modifiers |= OutputModifiers | StickyModifiers;

    if (InputModifiers != InputModifiersPrevious) {
        EventVector_Set(EventVector_LayerHolds);
    }
}

static void commitKeyState(key_state_t *keyState, bool active)
{
    WATCH_TRIGGER(keyState);

#if __ZEPHYR__ && !DEVICE_IS_UHK_DONGLE
    if (Shell.keyLog) {
        Log("Key %i %s", Utils_KeyStateToKeyId(keyState), active ? "down" : "up");
    }
#endif

    if (TestSwitches && active) {
        key_coordinates_t key = Utils_KeyIdToKeyCoordinates(Utils_KeyStateToKeyId(keyState));
        Ledmap_ActivateTestled(key.slotId, key.inSlotId);
        return;
    }

    if (PostponerCore_EventsShouldBeQueued()) {
        PostponerCore_TrackKeyEvent(keyState, active, 255);
    } else {
        KEY_TIMING(KeyTiming_RecordKeystroke(keyState, active, CurrentTime, CurrentTime));
        keyState->current = active;
    }
    Macros_WakeBecauseOfKeystateChange();
}

static inline void preprocessKeyState(key_state_t *keyState)
{
    uint8_t debounceTime = keyState->previous ? Cfg.DebounceTimePress : Cfg.DebounceTimeRelease;
    if (keyState->debouncing && (uint8_t)(CurrentTime - keyState->timestamp) >= debounceTime) {
        keyState->debouncing = false;
    }

    // read just once! Otherwise the key might get stuck
    bool hardwareState = keyState->hardwareSwitchState;
    if (!keyState->debouncing && keyState->debouncedSwitchState != hardwareState) {
        keyState->timestamp = CurrentTime;
        keyState->debouncing = true;
        keyState->debouncedSwitchState = hardwareState;

        commitKeyState(keyState, keyState->debouncedSwitchState);
    }

    if (keyState->debouncing) {
        uint8_t timeSinceActivation = (uint8_t)(CurrentTime - keyState->timestamp);
        EventScheduler_Schedule(CurrentTime + (debounceTime - timeSinceActivation), EventSchedulerEvent_NativeActions, "debouncing");
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
            Cfg.MouseMoveState.xOut = isEven ? -5 : 5;
        }
        EventVector_Set(EventVector_NativeActions);
        EventVector_Set(EventVector_SendUsbReports);
        EventVector_Set(EventVector_NativeActionReportsUsed);
    }
}

//This might be moved directly into the layer_switcher
static void handleLayerChanges() {
    static layer_id_t previousLayer = LayerId_Base;

    LayerSwitcher_UpdateHeldLayer();

    if(ActiveLayer != previousLayer) {
        previousLayer = ActiveLayer;
        StickyModifiers = 0;
        StickyModifiersNegative = 0;
        EventVector_Set(EventVector_SendUsbReports);
    }
}

static void updateActionStates() {
    uint8_t previousMods = NativeKeyboardReports.basic.modifiers | NativeKeyboardReports.inputModifiers;
    UsbReportUpdater_ResetKeyboardReports(&NativeKeyboardReports);

    LayerSwitcher_ResetHolds();

    memcpy(ActiveMouseStates, ToggledMouseStates, ACTIVE_MOUSE_STATES_COUNT);

    EventVector_Unset(EventVector_NativeActionReportsUsed);
    EventVector_Unset(EventVector_NativeActions);
    EventVector_Unset(EventVector_StateMatrix);
    EventVector_Unset(EventVector_NativeActionsPostponing);

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

                    if (CurrentPowerMode > PowerMode_LastAwake && CurrentPowerMode <= PowerMode_LightSleep) {
                        PowerMode_WakeHost();
                        PowerMode_ActivateMode(PowerMode_Awake, false);
                    }

                    if (Postponer_LastKeyLayer != 255 && PostponerCore_IsActive()) {
                        actionCache[slotId][keyId].action = CurrentKeymap[Postponer_LastKeyLayer][slotId][keyId];
                        Postponer_LastKeyLayer = 255;
                    } else if (Cfg.LayerConfig[ActiveLayer].modifierLayerMask != 0) {
                        if (CurrentKeymap[ActiveLayer][slotId][keyId].type != KeyActionType_None) {
                            actionCache[slotId][keyId].action = CurrentKeymap[ActiveLayer][slotId][keyId];
                            actionCache[slotId][keyId].modifierLayerMask = ActiveLayerModifierMask;
                        } else {
                            actionCache[slotId][keyId].action = CurrentKeymap[LayerId_Base][slotId][keyId];
                        }
                    } else {
                        actionCache[slotId][keyId].action = CurrentKeymap[ActiveLayer][slotId][keyId];
                    }
                    if (Postponer_LastKeyMods != 0) {
                        actionCache[slotId][keyId].action.keystroke.modifiers = Postponer_LastKeyMods;
                        Postponer_LastKeyMods = 0;
                    }
                    handleEventInterrupts(keyState);
                }

                cachedAction = &actionCache[slotId][keyId];
                actionBase = &CurrentKeymap[LayerId_Base][slotId][keyId];

                //apply base-layer holds
                applyLayerHolds(keyState, actionBase);

                //apply active-layer action
                ApplyKeyAction(keyState, cachedAction, actionBase, &NativeKeyboardReports);

                keyState->previous = keyState->current;
            }
        }
    }
    uint8_t currentMods = NativeKeyboardReports.basic.modifiers | NativeKeyboardReports.inputModifiers;
    if (currentMods != previousMods) {
        EventVector_Set(EventVector_SendUsbReports);
    }
}

static void updateActiveUsbReports(void)
{
    InputModifiersPrevious = InputModifiers;
    OutputModifiers = 0;
    static bool lastSomeonePostponing = false;

    PostponerCore_UpdatePostponedTime();

    if (EventVector_IsSet(EventVector_MacroEngine)) {
        EVENTLOOP_TIMING(EventloopTiming_WatchReset());
        Macros_ContinueMacro();
        EVENTLOOP_TIMING(EventloopTiming_Watch("macros"));
    }

    if (EventVector_IsSet(EventVector_LayerHolds)) {
        handleLayerChanges();
    }

    handleUsbStackTestMode();

    // if (EventVector_IsSet(EventVector_Posponer)) {
    //     PostponerCore_RunPostponedEvents();
    // }
    PostponerCore_RunPostponedEvents();

    if (EventVector_IsSet(EventVector_NativeActions | EventVector_StateMatrix)) {
        EVENTLOOP_TIMING(EventloopTiming_WatchReset());
        updateActionStates();
        EVENTLOOP_TIMING(EventloopTiming_Watch("action state update"));
    }

    if (EventVector_IsSet(EventVector_MouseKeys)) {
        EVENTLOOP_TIMING(EventloopTiming_WatchReset());
        MouseKeys_ProcessMouseActions();
        EVENTLOOP_TIMING(EventloopTiming_Watch("mouse keys"));
    }

    if (EventVector_IsSet(EventVector_MouseController)) {
        MouseController_ProcessMouseActions();
    }

    // If someone was postponing, the postponer has set itself to sleep. Wake it now.
    bool currentSomeonePostponing = EventVector_IsSet(EventVector_SomeonePostponing);
    if (lastSomeonePostponing && !currentSomeonePostponing && PostponerCore_IsNonEmpty()) {
        EventVector_Set(EventVector_Postponer);
    }
    lastSomeonePostponing = currentSomeonePostponing;
}

void justPreprocessInput(void) {
    for (uint8_t slotId=0; slotId<SLOT_COUNT; slotId++) {
        for (uint8_t keyId=0; keyId<MAX_KEY_COUNT_PER_MODULE; keyId++) {
            key_state_t *keyState = &KeyStates[slotId][keyId];

            preprocessKeyState(keyState);
        }
    }
}

uint32_t UsbReportUpdateCounter;

uint32_t UpdateUsbReports_LastUpdateTime = 0;
uint32_t lastBasicReportTime = 0;

static void sendActiveReports() {
    bool usbReportsChangedByAction = false;
    bool usbReportsChangedByAnything = false;

    // in case of usb error, this gets set back again
    EventVector_Unset(EventVector_SendUsbReports | EventVector_ResendUsbReports);

    if (UsbBasicKeyboardCheckReportReady() == kStatus_USB_Success) {
#ifdef __ZEPHYR__
        if (InputInterceptor_RegisterReport(ActiveUsbBasicKeyboardReport)) {
            SwitchActiveUsbBasicKeyboardReport();
        } else

#endif
        {
            MacroRecorder_RecordBasicReport(ActiveUsbBasicKeyboardReport);

            KEY_TIMING(KeyTiming_RecordReport(ActiveUsbBasicKeyboardReport));

            if(RuntimeMacroRecordingBlind) {
                //just switch reports without sending the report
                SwitchActiveUsbBasicKeyboardReport();
            } else {
                UsbBasicKeyboardSendActiveReport();
            }
            usbReportsChangedByAction = true;
            usbReportsChangedByAnything = true;
            lastBasicReportTime = CurrentTime;
            UsbReportUpdater_LastActivityTime = CurrentTime;
        }
    }

#ifdef __ZEPHYR__
    if (UsbMediaKeyboardCheckReportReady() == kStatus_USB_Success || UsbSystemKeyboardCheckReportReady() == kStatus_USB_Success) {
        UsbCompatibility_SendConsumerReport(ActiveUsbMediaKeyboardReport, ActiveUsbSystemKeyboardReport);
        SwitchActiveUsbMediaKeyboardReport();
        SwitchActiveUsbSystemKeyboardReport();
        UsbReportUpdater_LastActivityTime = CurrentTime;
        usbReportsChangedByAction = true;
        usbReportsChangedByAnything = true;
    }
#else
    if (UsbMediaKeyboardCheckReportReady() == kStatus_USB_Success) {
        UsbReportUpdateSemaphore |= 1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX;
        usb_status_t status = UsbMediaKeyboardAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX);
            EventVector_Set(EventVector_ResendUsbReports);
        }
        UsbReportUpdater_LastActivityTime = CurrentTime;
        usbReportsChangedByAction = true;
        usbReportsChangedByAnything = true;
    }

    if (UsbSystemKeyboardCheckReportReady() == kStatus_USB_Success) {
        UsbReportUpdateSemaphore |= 1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX;
        usb_status_t status = UsbSystemKeyboardAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX);
            EventVector_Set(EventVector_ResendUsbReports);
        }
        UsbReportUpdater_LastActivityTime = CurrentTime;
        usbReportsChangedByAction = true;
        usbReportsChangedByAnything = true;
    }
#endif

    bool usbMouseButtonsChanged = false;
    if (UsbMouseCheckReportReady(&usbMouseButtonsChanged) == kStatus_USB_Success) {
        UsbMouseSendActiveReport();
        UsbReportUpdater_LastActivityTime = CurrentTime;
        usbReportsChangedByAction |= usbMouseButtonsChanged;
        usbReportsChangedByAnything = true;
    }

    if (usbReportsChangedByAction) {
        Macros_SignalUsbReportsChange();
    }

    // If anything changed, trigger one more update to send zero reports
    // TODO: consider doing this depending on change of ReportsUsed mask(s) and actual module scans
    if (usbReportsChangedByAnything) {
        EventVector_Set(EventVector_SendUsbReports);
    }
}

static bool blockedByKeystrokeDelay() {
    static uint32_t postponedMasks = 0;
    if (CurrentTime < lastBasicReportTime + Cfg.KeystrokeDelay) {
        DISABLE_IRQ();
        postponedMasks |= EventScheduler_Vector & EventVector_MainTriggers;
        EventScheduler_Vector = (EventScheduler_Vector & ~EventVector_MainTriggers) | EventVector_KeystrokeDelayPostponing;
        ENABLE_IRQ();

        // Make sure to wake up postponer so that it can process the events.
        EventScheduler_Reschedule(lastBasicReportTime + Cfg.KeystrokeDelay, EventSchedulerEvent_Postponer, "keystroke delay");

        justPreprocessInput();
        return true;
    } else if (postponedMasks) {
        DISABLE_IRQ();
        EventScheduler_Vector = (EventScheduler_Vector & ~EventVector_KeystrokeDelayPostponing) | postponedMasks;
        postponedMasks = 0;
        ENABLE_IRQ();
    }
    return false;
}

void UpdateUsbReports(void)
{
    if (blockedByKeystrokeDelay()) {
        return;
    }

#if __ZEPHYR__ && (DEBUG_POSTPONER || DEBUG_EVENTLOOP_SCHEDULE)
    printk("========== new UpdateUsbReports cycle ==========\n");
#endif

    UpdateUsbReports_LastUpdateTime = CurrentTime;
    UsbReportUpdateCounter++;

    if (!EventVector_IsSet(EventVector_ResendUsbReports)) {
        updateActiveUsbReports();
    }

    if (EventVector_IsSet(EventVector_SendUsbReports | EventVector_ResendUsbReports)) {
        if (CurrentPowerMode < PowerMode_Lock) {
            mergeReports();
            sendActiveReports();
        } else {
            EventVector_Unset(EventVector_SendUsbReports | EventVector_ResendUsbReports);
        }
    }

    if (DisplaySleepModeActive || KeyBacklightSleepModeActive) {
        LedManager_UpdateSleepModes();
    }
}
