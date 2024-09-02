#include <math.h>
#include <string.h>
#include "key_action.h"
#include "layer.h"
#include "slave_protocol.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "slave_drivers/uhk_module_driver.h"
#include "timer.h"
#include "config_parser/parse_keymap.h"
#include "slave_drivers/touchpad_driver.h"
#include "mouse_controller.h"
#include "mouse_keys.h"
#include "slave_scheduler.h"
#include "layer_switcher.h"
#include "usb_report_updater.h"
#include "caret_config.h"
#include "debug.h"
#include "postponer.h"
#include "layer.h"
#include "config_manager.h"
#include "event_scheduler.h"

typedef struct {
    float x;
    float y;
    float speed;
    int16_t yInversion;
    int16_t xInversion;
    float speedDivisor;
    bool axisLockEnabled;
    float axisLockSkew;
    float axisLockFirstTickSkew;
    module_kinetic_state_t* ks;
    bool continuous;
} axis_locking_args_t;

module_kinetic_state_t leftModuleKineticState = {
    .currentModuleId = 0,
    .currentNavigationMode = 0,

    .caretAxis = CaretAxis_None,
    .caretFakeKeystate = {},
    .caretAction.action = { .type = KeyActionType_None },
    .xFractionRemainder = 0.0f,
    .yFractionRemainder = 0.0f,
    .lastUpdate = 0,
};

module_kinetic_state_t rightModuleKineticState = {
    .currentModuleId = 0,
    .currentNavigationMode = 0,

    .caretAxis = CaretAxis_None,
    .caretFakeKeystate = {},
    .caretAction.action = { .type = KeyActionType_None },
    .xFractionRemainder = 0.0f,
    .yFractionRemainder = 0.0f,
    .lastUpdate = 0,
};

usb_keyboard_reports_t MouseControllerKeyboardReports;
usb_mouse_report_t MouseControllerMouseReport;

static void processAxisLocking(axis_locking_args_t args);
static void handleRunningCaretModeAction(module_kinetic_state_t* ks);
static void handleSimpleRunningAction(module_kinetic_state_t* ks);

static float fastPow (float a, float b)
{
    // https://nic.schraudolph.org/pubs/Schraudolph99.pdf
    // https://martin.ankerl.com/2007/10/04/optimized-pow-approximation-for-java-and-c-c/
    if ( b == 0.0f ) {
        return 1.0f;
    } else {
        union {
            float f;
            int16_t x[2];
        } u = { a } ;

        u.x[1] = (int16_t)(b * (u.x[1] - 16249) + 16249);
        u.x[0] = 0;
        return u.f;
    }
}

static float computeModuleSpeed(float x, float y, uint8_t moduleId)
{
    //means that driver multiplier equals 1.0 at average speed midSpeed px/ms
    static float midSpeed = 3.0f;
    module_configuration_t *moduleConfiguration = GetModuleConfiguration(moduleId);
    float *currentSpeed = &moduleConfiguration->currentSpeed;

    if (x != 0 || y != 0) {
        static uint32_t lastUpdate = 0;
        uint32_t elapsedTime = CurrentTime - lastUpdate;
        float distance = sqrt(x*x + y*y);
        *currentSpeed = distance / (elapsedTime + 1);
        lastUpdate = CurrentTime;
    }

    float normalizedSpeed = *currentSpeed/midSpeed;
    return moduleConfiguration->baseSpeed + moduleConfiguration->speed*fastPow(normalizedSpeed, moduleConfiguration->xceleration);
}


typedef enum {
    State_Zero,
    State_Tap,
    State_TapAndHold,
    State_HoldContinuationDelay,
} tap_hold_state_t;

typedef enum {
    Event_None,
    Event_NewTap,
    Event_Timeout,
    Event_FingerIn,
    Event_FingerOut,
    Event_TapAndHold,
    Event_HoldContinuationTimeout,
} tap_hold_event_t;

typedef enum {
    Action_ResetTimer = 1,
    Action_Press = 2,
    Action_Release = 4,
    Action_Doubletap = 8,
    Action_ResetHoldContinuationTimeout = 16,
} tap_hold_action_t;

static tap_hold_state_t tapHoldAutomatonState = State_Zero;

static tap_hold_action_t tapHoldStateMachine(tap_hold_event_t event)
{
    switch (tapHoldAutomatonState) {
    // We are waiting for something to happen. Either tap, or native tapAndHold.
    case State_Zero:
        switch (event) {
        case Event_NewTap:
            tapHoldAutomatonState = State_Tap;
            return Action_ResetTimer | Action_Press;
        case Event_TapAndHold:
            tapHoldAutomatonState = State_TapAndHold;
            return Action_Press;
        default:
            return 0;
        }
    case State_Tap:
    // Last event was tap. We keep the tap active until timeout or next event, so that in case of doubletap-induced tap and hold, there is no release.
        switch (event) {
        case Event_NewTap:
            tapHoldAutomatonState = State_Tap;
            return Action_ResetTimer | Action_Doubletap;
        case Event_Timeout:
            tapHoldAutomatonState = State_Zero;
            return Action_Release;
        case Event_TapAndHold:
        case Event_FingerIn:
            tapHoldAutomatonState = State_TapAndHold;
            return 0;
        default:
            return 0;
        }
    case State_TapAndHold:
    // We entered tapAndHold state. This is kept alive either by noFingers == 1, or by native tapAndHold.
    // It still might turn out that finger event was just beginning of another tap.
        switch (event) {
        case Event_NewTap:
            tapHoldAutomatonState = State_Tap;
            return Action_ResetTimer | Action_Doubletap;
        case Event_FingerOut:
            if (Cfg.HoldContinuationTimeout == 0) {
                tapHoldAutomatonState = State_Zero;
                return Action_Release;
            } else {
                tapHoldAutomatonState = State_HoldContinuationDelay;
                return Action_ResetHoldContinuationTimeout;
            }
        default:
            return 0;
        }
    case State_HoldContinuationDelay:
        switch (event) {
        case Event_NewTap:
            tapHoldAutomatonState = State_Tap;
            return Action_ResetTimer | Action_Doubletap;
        case Event_TapAndHold:
        case Event_FingerIn:
            tapHoldAutomatonState = State_TapAndHold;
            return 0;
        case Event_HoldContinuationTimeout:
            tapHoldAutomatonState = State_Zero;
            return Action_Release;
        default:
            return 0;
        }
    }
    return 0;
}

static void feedTapHoldStateMachine(touchpad_events_t events)
{
    key_state_t* singleTap = &KeyStates[SlotId_RightModule][0];
    //todo: finetune this value. Low value will yield natural doubletaps, but requires fast doubletap to trigger tapAndHold.
    //      Or add artificial delay bellow.
    const uint16_t tapTimeout = 200;
    static uint32_t lastSingleTapTime = 0;
    static uint32_t continuationDelayStart = 0;
    static bool lastFinger = false;
    static bool lastSingleTapValue = false;
    static bool lastTapAndHoldValue = false;
    static bool lastSingleTapTimerActive = false;
    static bool holdContinuationTimerActive = false;
    tap_hold_action_t action = 0;
    tap_hold_event_t event = 0;

    if(!lastSingleTapValue && events.singleTap) {
        event = Event_NewTap;
        lastSingleTapValue = true;
    } else if (!lastTapAndHoldValue && events.tapAndHold) {
        event = Event_TapAndHold;

        lastTapAndHoldValue = true;
    } else if (lastFinger != (events.noFingers == 1)) {
        event = lastFinger ? Event_FingerOut : Event_FingerIn ;
        lastFinger = !lastFinger;
    } else if (lastSingleTapTimerActive && lastSingleTapTime + tapTimeout <= CurrentTime) {
        event = Event_Timeout;
        lastSingleTapTimerActive = false;
    } else if (holdContinuationTimerActive && continuationDelayStart + Cfg.HoldContinuationTimeout <= CurrentTime) {
        event = Event_HoldContinuationTimeout;
        holdContinuationTimerActive = false;
    }

    action = tapHoldStateMachine(event);

    if (action & Action_ResetTimer) {
        lastSingleTapTime = CurrentTime;
        lastSingleTapTimerActive = true;
    }
    if (action & Action_Release) {
        PostponerCore_TrackKeyEvent(singleTap, false, 0xff);
    }
    if (action & Action_Press) {
        PostponerCore_TrackKeyEvent(singleTap, true, 0xff);
    }
    if (action & Action_Doubletap) {
        PostponerCore_TrackKeyEvent(singleTap, false, 0xff);
        PostponerCore_TrackDelay(20);
        PostponerCore_TrackKeyEvent(singleTap, true, 0xff);
    }
    if (action & Action_ResetHoldContinuationTimeout) {
        continuationDelayStart = CurrentTime;
        holdContinuationTimerActive = true;
    }

    if (lastSingleTapTimerActive) {
        EventScheduler_Schedule(lastSingleTapTime + tapTimeout, EventSchedulerEvent_MouseController, "MouseController single tap timer");
    }

    if (holdContinuationTimerActive) {
        EventScheduler_Schedule(continuationDelayStart + Cfg.HoldContinuationTimeout, EventSchedulerEvent_MouseController, "MouseController hold continuation timer");
    }

    lastSingleTapValue &= events.singleTap;
    lastTapAndHoldValue &= events.tapAndHold;
}

static void processTouchpadActions(touchpad_events_t events) {

    if (events.singleTap || events.tapAndHold || tapHoldAutomatonState != State_Zero) {
        feedTapHoldStateMachine(events);
    }

    KeyStates[SlotId_RightModule][1].hardwareSwitchState = events.twoFingerTap;
    EventVector_Set(EventVector_StateMatrix);
}

static void progressZoomAction(module_kinetic_state_t* ks) {
    bool actionIsInactive = ks->caretFakeKeystate.current == false && ks->caretFakeKeystate.previous == false;

    // progress to next phase/mode
    if (actionIsInactive) {
        ks->zoomPhase++;
        uint8_t mode = 0;
        switch(ks->zoomPhase) {
        case 1:
            mode = NavigationMode_ZoomPc;
            break;
        case 2:
            mode = NavigationMode_ZoomMac;
            break;
        case 3:
            ks->zoomActive = false;
            ks->zoomPhase = false;
            ks->zoomSign = 0;
            return;
        }

        caret_configuration_t* currentCaretConfig = GetNavigationModeConfiguration(mode);
        caret_dir_action_t* dirActions = &currentCaretConfig->axisActions[CaretAxis_Vertical];
        ks->caretAction.action = ks->zoomSign > 0 ? dirActions->positiveAction : dirActions->negativeAction;
    }

    // progress current action
    handleSimpleRunningAction(ks);
}

static void handleNewCaretModeAction(caret_axis_t axis, uint8_t resultSign, int16_t value, module_kinetic_state_t* ks) {
    switch(ks->currentNavigationMode) {
        case NavigationMode_Cursor: {
            MouseControllerMouseReport.x += axis == CaretAxis_Horizontal ? value : 0;
            MouseControllerMouseReport.y -= axis == CaretAxis_Vertical ? value : 0;
            break;
        }
        case NavigationMode_Scroll: {
            MouseControllerMouseReport.wheelX += axis == CaretAxis_Horizontal ? value : 0;
            MouseControllerMouseReport.wheelY += axis == CaretAxis_Vertical ? value : 0;
            break;
        }
        case NavigationMode_ZoomMac:
        case NavigationMode_ZoomPc:
        case NavigationMode_Media:
        case NavigationMode_Caret: {
            caret_configuration_t* currentCaretConfig = GetNavigationModeConfiguration(ks->currentNavigationMode);
            caret_dir_action_t* dirActions = &currentCaretConfig->axisActions[ks->caretAxis];
            ks->caretAction.action = resultSign > 0 ? dirActions->positiveAction : dirActions->negativeAction;
            ks->caretFakeKeystate.current = true;
            ApplyKeyAction(&ks->caretFakeKeystate, &ks->caretAction, &ks->caretAction.action, &MouseControllerKeyboardReports);
            break;
        }
        case NavigationMode_Zoom:
            if (axis != CaretAxis_Vertical) {
                return;
            }
            ks->zoomActive = 1;
            ks->zoomPhase = 0;
            ks->zoomSign = resultSign;
            progressZoomAction(ks);
            break;
        case NavigationMode_None:
            break;
    }
}

static void handleSimpleRunningAction(module_kinetic_state_t* ks) {
    bool tmp = ks->caretFakeKeystate.current;
    ks->caretFakeKeystate.current = !ks->caretFakeKeystate.previous;
    ks->caretFakeKeystate.previous = tmp;
    ApplyKeyAction(&ks->caretFakeKeystate, &ks->caretAction, &ks->caretAction.action, &MouseControllerKeyboardReports);
}

static void handleRunningCaretModeAction(module_kinetic_state_t* ks) {
    if (ks->zoomActive) {
        progressZoomAction(ks);
    } else {
        handleSimpleRunningAction(ks);
    }
    EventVector_Set(EventVector_MouseController);
}

static bool caretModeActionIsRunning(module_kinetic_state_t* ks) {
    return ks->caretFakeKeystate.current || ks->caretFakeKeystate.previous || ks->zoomActive;
}

static void processAxisLocking(
    axis_locking_args_t args
) {
    //optimize this out if nothing is going on
    if (args.x == 0 && args.y == 0 && args.ks->caretAxis == CaretAxis_None) {
        return;
    }

    //unpack args for better readability
    float x = args.x;
    float y = args.y;
    float speed = args.speed;
    int16_t yInversion = args.yInversion;
    int16_t xInversion = args.xInversion;
    float speedDivisor = args.speedDivisor;
    bool axisLockEnabled = args.axisLockEnabled;
    float axisLockSkew = args.axisLockSkew;
    float axisLockSkewFirstTick = args.axisLockFirstTickSkew;
    module_kinetic_state_t *ks = args.ks;
    bool continuous  = args.continuous;

    //unlock axis if inactive for some time and re-activate tick trashold`
    if (x != 0 || y != 0) {
        if (
                Timer_GetElapsedTime(&ks->lastUpdate) > 500
                && ks->caretAxis != CaretAxis_None
                && ks->zoomActive == false
                && ks->caretFakeKeystate.current == false
                && ks->caretFakeKeystate.previous == false
        ) {
            ks->xFractionRemainder = 0;
            ks->yFractionRemainder = 0;
            ks->caretAxis = CaretAxis_None;
        }

        ks->lastUpdate = CurrentTime;
    }

    // caretAxis tries to lock to one direction, therefore we "skew" the other one
    float caretXModeMultiplier;
    float caretYModeMultiplier;

    if (!axisLockEnabled) {
        caretXModeMultiplier = 1.0f;
        caretYModeMultiplier =  1.0f;
    }
    else if (ks->caretAxis == CaretAxis_None) {
        caretXModeMultiplier = axisLockSkewFirstTick;
        caretYModeMultiplier = axisLockSkewFirstTick;
    } else {
        caretXModeMultiplier = ks->caretAxis == CaretAxis_Horizontal ? 1.0f : axisLockSkew;
        caretYModeMultiplier = ks->caretAxis == CaretAxis_Vertical ? 1.0f : axisLockSkew;
    }

    ks->xFractionRemainder += x * speed / speedDivisor * caretXModeMultiplier;
    ks->yFractionRemainder += y * speed / speedDivisor * caretYModeMultiplier;

    // Start a new action (new "tick"), unless there is an action in progress.
    if (!caretModeActionIsRunning(ks)) {
        // determine default axis
        caret_axis_t axisCandidate;

        if ( ks->caretAxis == CaretAxis_Inactive ) {
            float absX = ABS(ks->xFractionRemainder);
            float absY = ABS(ks->yFractionRemainder);
            axisCandidate = absX > absY ? CaretAxis_Horizontal : CaretAxis_Vertical;
        } else {
            axisCandidate = ks->caretAxis;
        }

        // setup the indirections

        float* axisFractionRemainders [CaretAxis_Count] = {&ks->xFractionRemainder, &ks->yFractionRemainder};
        float axisIntegerParts [CaretAxis_Count] = { 0, 0 };

        // determine integer parts

        modff(ks->xFractionRemainder, &axisIntegerParts[CaretAxis_Horizontal]);
        modff(ks->yFractionRemainder, &axisIntegerParts[CaretAxis_Vertical]);

        // pick axis to apply action on, if possible - check previously active axis first
        if ( axisIntegerParts[axisCandidate] != 0 ) {
            axisCandidate = axisCandidate;
        } else if ( axisIntegerParts[1 - axisCandidate] != 0 ) {
            axisCandidate = 1 - axisCandidate;
        } else {
            axisCandidate = CaretAxis_None;
        }

        // handle the action
        if ( axisCandidate < CaretAxis_Count ) {
            float sgn = axisIntegerParts[axisCandidate] > 0 ? 1 : -1;
            int8_t currentAxisInversion = axisCandidate == CaretAxis_Vertical ? yInversion : xInversion;
            float consumedAmount = continuous ? axisIntegerParts[axisCandidate] : sgn;
            *axisFractionRemainders[axisCandidate] -= consumedAmount;

            //always zero primary axis - experimental
            // TODO: this slows down maximum rate, as right half processing is much faster than module refresh rate.
            //       We should solve this by zeroing remainder before next event, rather than at the end of previous one.
            //       In order for this to work, we will have to acknowledge received zeroes.
            *axisFractionRemainders[axisCandidate] = 0.0f;

            if (axisLockEnabled) {
                // if not axis locking, than allow accumulation of secondary axis
                *axisFractionRemainders[1 - axisCandidate] = 0.0f;
            }
            if (ks->caretAxis == CaretAxis_None) {
                // zero state after first tick, in order to support
                // skews greater than 1
                *axisFractionRemainders[CaretAxis_Vertical] = 0.0f;
                *axisFractionRemainders[CaretAxis_Horizontal] = 0.0f;
            }
            ks->caretAxis = axisCandidate;

            if (!continuous) {
                EventVector_Set(EventVector_MouseController);
            }

            handleNewCaretModeAction(ks->caretAxis, sgn*currentAxisInversion, consumedAmount*currentAxisInversion, ks);
        }
    }
}

static void processModuleKineticState(
        float x,
        float y,
        module_configuration_t* moduleConfiguration,
        module_kinetic_state_t* ks,
        uint8_t forcedNavigationMode
 ) {
    float speed;

    bool moduleYInversion = ks->currentModuleId == ModuleId_KeyClusterLeft || ks->currentModuleId == ModuleId_TouchpadRight;
    bool scrollYInversion = moduleConfiguration->invertScrollDirectionY && ks->currentNavigationMode == NavigationMode_Scroll;
    bool scrollXInversion = moduleConfiguration->invertScrollDirectionX && ks->currentNavigationMode == NavigationMode_Scroll;
    int16_t yInversion = moduleYInversion != scrollYInversion ? -1 : 1;
    int16_t xInversion = scrollXInversion ? -1 : 1;


    speed = computeModuleSpeed(x, y, ks->currentModuleId);

    if (ActiveMouseStates[SerializedMouseAction_Accelerate] ) {
        speed *= 2.0f;
    }
    if (ActiveMouseStates[SerializedMouseAction_Decelerate] ) {
        speed /= 2.0f;
    }

    switch (ks->currentNavigationMode) {
        case NavigationMode_Cursor: {
            if (!moduleConfiguration->cursorAxisLock) {
                float xIntegerPart;
                float yIntegerPart;

                ks->xFractionRemainder = modff(ks->xFractionRemainder + x * speed, &xIntegerPart);
                ks->yFractionRemainder = modff(ks->yFractionRemainder + y * speed, &yIntegerPart);

                MouseControllerMouseReport.x += xInversion*xIntegerPart;
                MouseControllerMouseReport.y -= yInversion*yIntegerPart;
            } else {
                processAxisLocking((axis_locking_args_t) {
                    .x = x,
                    .y = y,
                    .speed = speed,
                    .yInversion = yInversion,
                    .xInversion = xInversion,
                    .speedDivisor = 1.0,
                    .axisLockEnabled = true,
                    .axisLockSkew = moduleConfiguration->axisLockSkew,
                    .axisLockFirstTickSkew = moduleConfiguration->axisLockFirstTickSkew,
                    .ks = ks,
                    .continuous = true,
                });
            }
            break;
        }
        case NavigationMode_Scroll:  {
            if (!moduleConfiguration->scrollAxisLock) {
                float xIntegerPart;
                float yIntegerPart;

                ks->xFractionRemainder = modff(ks->xFractionRemainder + x * speed / moduleConfiguration->scrollSpeedDivisor, &xIntegerPart);
                ks->yFractionRemainder = modff(ks->yFractionRemainder + y * speed / moduleConfiguration->scrollSpeedDivisor, &yIntegerPart);

                MouseControllerMouseReport.wheelX += xInversion*xIntegerPart;
                MouseControllerMouseReport.wheelY += yInversion*yIntegerPart;
            } else {

                processAxisLocking((axis_locking_args_t) {
                    .x = x,
                    .y = y,
                    .speed = speed,
                    .yInversion = yInversion,
                    .xInversion = xInversion,
                    .speedDivisor = moduleConfiguration->scrollSpeedDivisor,
                    .axisLockEnabled = true,
                    .axisLockSkew = moduleConfiguration->axisLockSkew,
                    .axisLockFirstTickSkew = moduleConfiguration->axisLockFirstTickSkew,
                    .ks = ks,
                    .continuous = true,
                });
            }
            break;
        }
        case NavigationMode_Zoom:
        case NavigationMode_ZoomPc:
        case NavigationMode_ZoomMac:
        case NavigationMode_Media:
        case NavigationMode_Caret:;
            // forced zoom = touchpad pinch zoom; it needs special coefficient
            // forced scroll = touchpad scroll;
            bool isPinchGesture = forcedNavigationMode == NavigationMode_Zoom;
            float speedDivisor = isPinchGesture ? moduleConfiguration->pinchZoomSpeedDivisor : moduleConfiguration->caretSpeedDivisor;
            processAxisLocking((axis_locking_args_t) {
                .x = x,
                .y = y,
                .speed = speed,
                .yInversion = yInversion,
                .xInversion = xInversion,
                .speedDivisor = speedDivisor,
                .axisLockEnabled = moduleConfiguration->caretAxisLock,
                .axisLockSkew = moduleConfiguration->axisLockSkew,
                .axisLockFirstTickSkew = moduleConfiguration->axisLockFirstTickSkew,
                .ks = ks,
                .continuous = false,
            });
            break;
        case NavigationMode_None:
            break;

    }
}

static void resetKineticModuleState(module_kinetic_state_t* kineticState)
{
    kineticState->currentModuleId = 0;
    kineticState->currentNavigationMode = 0;
    kineticState->caretAxis = CaretAxis_None;
    kineticState->xFractionRemainder = 0.0f;
    kineticState->yFractionRemainder = 0.0f;
    kineticState->lastUpdate = 0;

    //leave caretFakeKeystate & caretAction intact - this will ensure that any ongoing key action will complete properly
}

static layer_id_t determineEffectiveLayer() {
    if (IS_MODIFIER_LAYER(ActiveLayer)) {
        return LayerId_Base;
    } else {
        return ActiveLayer;
    }
}

static module_kinetic_state_t* getKineticState(uint8_t moduleId)
{
    return moduleId == ModuleId_KeyClusterLeft ? &leftModuleKineticState : &rightModuleKineticState;
}

static void processModuleActions(
        module_kinetic_state_t* ks,
        uint8_t moduleId,
        float x,
        float y,
        uint8_t forcedNavigationMode
) {
    module_configuration_t *moduleConfiguration = GetModuleConfiguration(moduleId);

    navigation_mode_t navigationMode;

    if(forcedNavigationMode == 0xFF) {
        navigationMode = moduleConfiguration->navigationModes[determineEffectiveLayer()];
    } else if (forcedNavigationMode == NavigationMode_Zoom) {
        // set kineticState navigation mode to the actual target mode, but forward
        // forcedNavigationMode == NavigationMode_Zoom to unambiguously signal that
        // the mode is actually touchpad pinch zoom gesture
        navigationMode = Cfg.TouchpadPinchZoomMode;
    } else {
        navigationMode = forcedNavigationMode;
    }

    bool moduleIsActive = x != 0 || y != 0;
    bool keystateOwnerDiffers = ks->currentModuleId != moduleId || ks->currentNavigationMode != navigationMode;
    bool keyActionIsNotActive = true
        && ks->caretFakeKeystate.current == false
        && ks->caretFakeKeystate.previous == false
        && ks->zoomActive == 0;

    if (moduleIsActive && keystateOwnerDiffers && keyActionIsNotActive) {
        // Currently, we share the state among (different right-hand) modules &
        // navigation modes, and reset it whenever the user starts to use other
        // mode.
        resetKineticModuleState(ks);

        ks->currentModuleId = moduleId;
        ks->currentNavigationMode = navigationMode;
    }

    if(moduleConfiguration->swapAxes) {
        float tmp = x;
        x = y;
        y = tmp;
    }

    //we want to process kinetic state even if x == 0 && y == 0, at least as
    //long as caretAxis != CaretAxis_None because of fake key states that may
    //be active.
    processModuleKineticState(x, y, moduleConfiguration, ks, forcedNavigationMode);
}

bool canWeRun(module_kinetic_state_t* ks)
{
    if (caretModeActionIsRunning(ks)) {
        EventVector_Set(EventVector_MouseController);
        return false;
    }
    if (StickyModifiers) {
        StickyModifiers = 0;
        StickyModifiersNegative = 0;
        EventVector_Set(EventVector_MouseController);
        return false;
    }
    if (Postponer_MouseBlocked) {
        PostponerExtended_RequestUnblockMouse();
        EventVector_Set(EventVector_MouseController);
        return false;
    }
    return true;
}

void MouseController_ProcessMouseActions()
{
    EventVector_Unset(EventVector_MouseController);

    memset(&MouseControllerMouseReport, 0, sizeof(MouseControllerMouseReport));
    memset(&MouseControllerKeyboardReports.basic, 0, sizeof MouseControllerKeyboardReports.basic);
    memset(&MouseControllerKeyboardReports.media, 0, sizeof MouseControllerKeyboardReports.media);
    memset(&MouseControllerKeyboardReports.system, 0, sizeof MouseControllerKeyboardReports.system);

    if (Slaves[SlaveId_RightTouchpad].isConnected) {
        module_kinetic_state_t *ks = getKineticState(ModuleId_TouchpadRight);

        if (caretModeActionIsRunning(ks)) {
            handleRunningCaretModeAction(ks);
        }

        bool eventsIsNonzero = memcmp(&TouchpadEvents, &ZeroTouchpadEvents, sizeof TouchpadEvents) != 0;
        if (!eventsIsNonzero || (eventsIsNonzero && canWeRun(ks))) {
            //eventsIsNonzero is needed for touchpad action state automaton timer
#ifndef __ZEPHYR__
            __disable_irq();
#endif
            touchpad_events_t events = TouchpadEvents;
            TouchpadEvents.zoomLevel = 0;
            TouchpadEvents.wheelX = 0;
            TouchpadEvents.wheelY = 0;
            TouchpadEvents.x = 0;
            TouchpadEvents.y = 0;
            // note that not all fields are resetted and that's correct
#ifndef __ZEPHYR__
            __enable_irq();
#endif
            processTouchpadActions(events);

            processModuleActions(ks, ModuleId_TouchpadRight, (int16_t)events.x, (int16_t)events.y, 0xFF);
            processModuleActions(ks, ModuleId_TouchpadRight, (int16_t)events.wheelX, (int16_t)events.wheelY, NavigationMode_Scroll);
            processModuleActions(ks, ModuleId_TouchpadRight, 0, (int16_t)events.zoomLevel, NavigationMode_Zoom);
        }
    }

    for (uint8_t moduleSlotId=0; moduleSlotId<UHK_MODULE_MAX_SLOT_COUNT; moduleSlotId++) {
        uhk_module_state_t *moduleState = UhkModuleStates + moduleSlotId;

        if (moduleState->moduleId == ModuleId_Unavailable || moduleState->pointerCount == 0) {
            continue;
        }

        module_kinetic_state_t *ks = getKineticState(moduleState->moduleId);

        if (caretModeActionIsRunning(ks)) {
            handleRunningCaretModeAction(ks);
        }


        bool eventsIsNonzero = moduleState->pointerDelta.x || moduleState->pointerDelta.y;
        if (eventsIsNonzero && canWeRun(ks)) {
#ifndef __ZEPHYR__
            __disable_irq();
#endif
            // Gcc compiles those int16_t assignments as sequences of
            // single-byte instructions, therefore we need to make the
            // sequence atomic.
            int16_t x = moduleState->pointerDelta.x;
            int16_t y = moduleState->pointerDelta.y;
            moduleState->pointerDelta.x = 0;
            moduleState->pointerDelta.y = 0;
#ifndef __ZEPHYR__
            __enable_irq();
#endif

            processModuleActions(ks, moduleState->moduleId, x, y, 0xFF);
        }
    }

    bool keyboardReportsUsed = caretModeActionIsRunning(&leftModuleKineticState) || caretModeActionIsRunning(&rightModuleKineticState);
    bool mouseReportsUsed = MouseControllerMouseReport.x || MouseControllerMouseReport.y || MouseControllerMouseReport.wheelX || MouseControllerMouseReport.wheelY || MouseControllerMouseReport.buttons;
    EventVector_SetValue(EventVector_MouseControllerKeyboardReportsUsed, keyboardReportsUsed);
    EventVector_SetValue(EventVector_MouseControllerMouseReportsUsed, mouseReportsUsed);

    if (keyboardReportsUsed || mouseReportsUsed) {
        EventVector_Set(EventVector_ReportsChanged);
    }
}
