#include <math.h>
#include "key_action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "peripherals/test_led.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "timer.h"
#include "config_parser/parse_keymap.h"
#include "usb_commands/usb_command_get_debug_buffer.h"
#include "arduino_hid/ConsumerAPI.h"
#include "secondary_role_driver.h"
#include "slave_drivers/touchpad_driver.h"
#include "mouse_controller.h"
#include "slave_scheduler.h"
#include "layer_switcher.h"
#include "usb_report_updater.h"
#include "caret_config.h"
#include "keymap.h"
#include "macros.h"
#include "debug.h"
#include "postponer.h"
#include "secondary_role_driver.h"

static uint32_t mouseUsbReportUpdateTime = 0;
static uint32_t mouseElapsedTime;

uint8_t ActiveMouseStates[ACTIVE_MOUSE_STATES_COUNT];
uint8_t ToggledMouseStates[ACTIVE_MOUSE_STATES_COUNT];

bool CompensateDiagonalSpeed = false;

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
    .axisSkew = 1.0f,
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
    .axisSkew = 1.0f,
};

module_kinetic_state_t moduleKineticState = {
    .currentModuleId = 0,
    .currentNavigationMode = 0,

    .caretAxis = CaretAxis_None,
    .caretFakeKeystate = {},
    .caretAction = &CurrentKeymap[0][0][0],
    .xFractionRemainder = 0.0f,
    .yFractionRemainder = 0.0f,
    .lastUpdate = 0,
};

static void processAxisLocking(float x, float y, float speed, int16_t yInversion, float speedDivisor, float axisLockSkew, float axisLockSkewFirstTick, module_kinetic_state_t* ks, bool continuous);
static void handleRunningCaretModeAction(module_kinetic_state_t* ks);

static void updateOneDirectionSign(int8_t* sign, int8_t expectedSign, uint8_t expectedState, uint8_t otherState) {
    if (*sign == expectedSign && !ActiveMouseStates[expectedState]) {
        *sign = ActiveMouseStates[otherState] ? -expectedSign : 0;
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
void MouseController_ActivateDirectionSigns(uint8_t state) {
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

    if (!kineticState->wasMoveAction && !ActiveMouseStates[SerializedMouseAction_Decelerate]) {
        kineticState->currentSpeed = initialSpeed;
    }

    bool doublePressedStateExists = ActiveMouseStates[kineticState->upState] > 1 ||
            ActiveMouseStates[kineticState->downState] > 1 ||
            ActiveMouseStates[kineticState->leftState] > 1 ||
            ActiveMouseStates[kineticState->rightState] > 1;

    bool isMoveAction = ActiveMouseStates[kineticState->upState] ||
                        ActiveMouseStates[kineticState->downState] ||
                        ActiveMouseStates[kineticState->leftState] ||
                        ActiveMouseStates[kineticState->rightState];

    mouse_speed_t mouseSpeed = MouseSpeed_Normal;
    if (ActiveMouseStates[SerializedMouseAction_Accelerate] || doublePressedStateExists) {
        kineticState->targetSpeed = acceleratedSpeed;
        mouseSpeed = MouseSpeed_Accelerated;
    } else if (ActiveMouseStates[SerializedMouseAction_Decelerate]) {
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

        kineticState->xSum += distance * kineticState->horizontalStateSign * kineticState->axisSkew;
        kineticState->ySum += distance * kineticState->verticalStateSign / kineticState->axisSkew;

        // Update horizontal state

        bool horizontalMovement = kineticState->horizontalStateSign != 0;

        float xSumInt;
        float xSumFrac = modff(kineticState->xSum, &xSumInt);
        kineticState->xSum = xSumFrac;
        kineticState->xOut = xSumInt;

        // Handle the first scroll tick.
        if (kineticState->isScroll && !kineticState->wasMoveAction && kineticState->xOut == 0 && horizontalMovement) {
            kineticState->xOut = ActiveMouseStates[kineticState->leftState] ? -1 : 1;
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
            kineticState->yOut = ActiveMouseStates[kineticState->upState] ? -1 : 1;
            kineticState->ySum = 0;
        }
    } else {
        kineticState->currentSpeed = 0;
    }

    kineticState->prevMouseSpeed = mouseSpeed;
    kineticState->wasMoveAction = isMoveAction;
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
    return moduleConfiguration->baseSpeed + moduleConfiguration->speed*(float)pow(normalizedSpeed, moduleConfiguration->acceleration);
}


typedef enum {
    State_Zero,
    State_Tap,
    State_TapAndHold
} tap_hold_state_t;

typedef enum {
    Event_None,
    Event_NewTap,
    Event_Timeout,
    Event_FingerIn,
    Event_FingerOut,
    Event_TapAndHold,
} tap_hold_event_t;

typedef enum {
    Action_ResetTimer = 1,
    Action_Press = 2,
    Action_Release = 4
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
            return Action_ResetTimer | Action_Release | Action_Press;
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
            return Action_ResetTimer | Action_Release | Action_Press;
        case Event_FingerOut:
            tapHoldAutomatonState = State_Zero;
            return Action_Release;
        default:
            return 0;
        }
    }
    return 0;
}

static void feedTapHoldStateMachine()
{
    key_state_t* singleTap = &KeyStates[SlotId_RightModule][0];
    //todo: finetune this value. Low value will yield natural doubletaps, but requires fast doubletap to trigger tapAndHold.
    //      Or add artificial delay bellow.
    const uint16_t tapTimeout = 200;
    static uint32_t lastSingleTapTime = 0;
    static bool lastFinger = false;
    static bool lastSingleTapValue = false;
    static bool lastTapAndHoldValue = false;
    tap_hold_action_t action = 0;
    tap_hold_event_t event = 0;

    if(!lastSingleTapValue && TouchpadEvents.singleTap) {
        event = Event_NewTap;
        lastSingleTapValue = true;
    } else if (!lastTapAndHoldValue && TouchpadEvents.tapAndHold) {
        event = Event_TapAndHold;
        lastTapAndHoldValue = true;
    } else if (lastFinger != (TouchpadEvents.noFingers == 1)) {
        event = lastFinger ? Event_FingerOut : Event_FingerIn ;
        lastFinger = !lastFinger;
    } else if(lastSingleTapTime + tapTimeout < CurrentTime) {
        event = Event_Timeout;
    }

    action = tapHoldStateMachine(event);

    if (action & Action_ResetTimer) {
        lastSingleTapTime = CurrentTime;
    }
    if (action & Action_Release) {
        PostponerCore_TrackKeyEvent(singleTap, false);
        /** TODO: consider adding an explicit delay here - at least my linux machine does not like the idea of releases shorther than 25 ms */
    }
    if (action & Action_Press) {
        PostponerCore_TrackKeyEvent(singleTap, true);
    }

    lastSingleTapValue &= TouchpadEvents.singleTap;
    lastTapAndHoldValue &= TouchpadEvents.tapAndHold;
}

static void processTouchpadActions() {

    if (TouchpadEvents.singleTap || TouchpadEvents.tapAndHold || tapHoldAutomatonState != State_Zero) {
        feedTapHoldStateMachine();
    }

    KeyStates[SlotId_RightModule][1].hardwareSwitchState = TouchpadEvents.twoFingerTap;
}

static void handleNewCaretModeAction(caret_axis_t axis, uint8_t resultSign, int16_t value, module_kinetic_state_t* ks) {
    switch(ks->currentNavigationMode) {
        case NavigationMode_Cursor: {
            ActiveUsbMouseReport->x += axis == CaretAxis_Horizontal ? value : 0;
            ActiveUsbMouseReport->y -= axis == CaretAxis_Vertical ? value : 0;
            break;
        }
        case NavigationMode_Scroll: {
            ActiveUsbMouseReport->wheelX += axis == CaretAxis_Horizontal ? value : 0;
            ActiveUsbMouseReport->wheelY += axis == CaretAxis_Vertical ? value : 0;
            break;
        }
        case NavigationMode_Zoom:
        case NavigationMode_Media:
        case NavigationMode_Caret: {
            caret_configuration_t* currentCaretConfig = GetModuleCaretConfiguration(ks->currentModuleId, ks->currentNavigationMode);
            caret_dir_action_t* dirActions = &currentCaretConfig->axisActions[ks->caretAxis];
            ks->caretAction = resultSign > 0 ? &dirActions->positiveAction : &dirActions->negativeAction;
            ks->caretFakeKeystate.current = true;
            ApplyKeyAction(&ks->caretFakeKeystate, ks->caretAction, ks->caretAction);
            break;
        }
        case NavigationMode_None:
            break;
    }
}

static void handleRunningCaretModeAction(module_kinetic_state_t* ks) {
    bool tmp = ks->caretFakeKeystate.current;
    ks->caretFakeKeystate.current = !ks->caretFakeKeystate.previous;
    ks->caretFakeKeystate.previous = tmp;
    ApplyKeyAction(&ks->caretFakeKeystate, ks->caretAction, ks->caretAction);
}

static void processAxisLocking(float x, float y, float speed, int16_t yInversion, float speedDivisor, float axisLockSkew, float axisLockSkewFirstTick, module_kinetic_state_t* ks, bool continuous)
{
    //optimize this out if nothing is going on
    if (x == 0 && y == 0 && ks->caretAxis == CaretAxis_None) {
        return;
    }

    //unlock axis if inactive for some time and re-activate tick trashold`
    if (x != 0 || y != 0) {
        if (Timer_GetElapsedTime(&ks->lastUpdate) > 500 && ks->caretAxis != CaretAxis_None) {
            ks->xFractionRemainder = 0;
            ks->yFractionRemainder = 0;
            ks->caretAxis = CaretAxis_None;
        }

        ks->lastUpdate = CurrentTime;
    }

    // caretAxis tries to lock to one direction, therefore we "skew" the other one
    float caretXModeMultiplier;
    float caretYModeMultiplier;

    if(ks->caretAxis == CaretAxis_None) {
        caretXModeMultiplier = axisLockSkewFirstTick;
        caretYModeMultiplier = axisLockSkewFirstTick;
    } else {
        caretXModeMultiplier = ks->caretAxis == CaretAxis_Horizontal ? 1.0f : axisLockSkew;
        caretYModeMultiplier = ks->caretAxis == CaretAxis_Vertical ? 1.0f : axisLockSkew;
    }

    ks->xFractionRemainder += x * speed / speedDivisor * caretXModeMultiplier;
    ks->yFractionRemainder += y * speed / speedDivisor * caretYModeMultiplier;


    //If there is an ongoing action, just handle that action via a fake state. Ensure that full lifecycle of a key gets executed.
    if (ks->caretFakeKeystate.current || ks->caretFakeKeystate.previous) {
        handleRunningCaretModeAction(ks);
    }
    //If we want to start a new action (new "tick")
    else {
        // determine current axis properties and setup indirections for easier handling
        caret_axis_t axisCandidate = ks->caretAxis == CaretAxis_Inactive ? CaretAxis_Vertical : ks->caretAxis;
        float* axisFractionRemainders [CaretAxis_Count] = {&ks->xFractionRemainder, &ks->yFractionRemainder};
        float axisIntegerParts [CaretAxis_Count] = { 0, 0 };

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
            ks->caretAxis = axisCandidate;
            float sgn = axisIntegerParts[axisCandidate] > 0 ? 1 : -1;
            int8_t currentAxisInversion = axisCandidate == CaretAxis_Vertical ? yInversion : 1;
            float consumedAmount = continuous ? axisIntegerParts[axisCandidate] : sgn;
            *axisFractionRemainders[1 - axisCandidate] = 0.0f;
            *axisFractionRemainders[axisCandidate] -= consumedAmount;
            handleNewCaretModeAction(ks->caretAxis, sgn*currentAxisInversion, consumedAmount*currentAxisInversion, ks);
        }
    }
}

static void processModuleKineticState(float x, float y, module_configuration_t* moduleConfiguration, module_kinetic_state_t* ks) {
    float speed;

    int16_t yInversion = ks->currentModuleId == ModuleId_KeyClusterLeft || ks->currentModuleId == ModuleId_TouchpadRight ? -1 : 1;

    speed = computeModuleSpeed(x, y, ks->currentModuleId);

    switch (ks->currentNavigationMode) {
        case NavigationMode_Cursor: {
            if (!moduleConfiguration->cursorAxisLock) {
                float xIntegerPart;
                float yIntegerPart;

                ks->xFractionRemainder = modff(ks->xFractionRemainder + x * speed, &xIntegerPart);
                ks->yFractionRemainder = modff(ks->yFractionRemainder + y * speed, &yIntegerPart);

                ActiveUsbMouseReport->x += xIntegerPart;
                ActiveUsbMouseReport->y -= yInversion*yIntegerPart;
            } else {
                processAxisLocking(x, y, speed, yInversion, 1.0f, moduleConfiguration->axisLockSkew, moduleConfiguration->axisLockSkewFirstTick, ks, true);
            }
            break;
        }
        case NavigationMode_Scroll:  {
            if (!moduleConfiguration->scrollAxisLock) {
                float xIntegerPart;
                float yIntegerPart;

                ks->xFractionRemainder = modff(ks->xFractionRemainder + x * speed / moduleConfiguration->scrollSpeedDivisor, &xIntegerPart);
                ks->yFractionRemainder = modff(ks->yFractionRemainder + y * speed / moduleConfiguration->scrollSpeedDivisor, &yIntegerPart);

                ActiveUsbMouseReport->wheelX += xIntegerPart;
                ActiveUsbMouseReport->wheelY += yInversion*yIntegerPart;
            } else {
                processAxisLocking(x, y, speed, yInversion, moduleConfiguration->scrollSpeedDivisor, moduleConfiguration->axisLockSkew, moduleConfiguration->axisLockSkewFirstTick, ks, true);
            }
            break;
        }
        case NavigationMode_Zoom:
            processAxisLocking(x, y, speed, yInversion, moduleConfiguration->zoomSpeedDivisor, moduleConfiguration->axisLockSkew, moduleConfiguration->axisLockSkewFirstTick, ks, false);
            break;
        case NavigationMode_Media:
        case NavigationMode_Caret:
            processAxisLocking(x, y, speed, yInversion, moduleConfiguration->caretSpeedDivisor, moduleConfiguration->axisLockSkew, moduleConfiguration->axisLockSkewFirstTick, ks, false);
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
    bool secondaryRoleResolutionInProgress = ActiveLayer == LayerId_Base && IS_SECONDARY_ROLE_LAYER_SWITCHER(SecondaryRolePreview);

    return secondaryRoleResolutionInProgress ? SECONDARY_ROLE_LAYER_TO_LAYER_ID(SecondaryRolePreview) : ActiveLayer;
}

static void processModuleActions(uint8_t moduleId, float x, float y, uint8_t forcedNavigationMode)
{
    module_configuration_t *moduleConfiguration = GetModuleConfiguration(moduleId);

    navigation_mode_t navigationMode;

    if(forcedNavigationMode == 0xFF) {
        navigationMode = moduleConfiguration->navigationModes[determineEffectiveLayer()];
    } else {
        navigationMode = forcedNavigationMode;
    }

    bool moduleIsActive = x != 0 || y != 0;
    bool keystateOwnerDiffers = moduleKineticState.currentModuleId != moduleId || moduleKineticState.currentNavigationMode != navigationMode;
    bool keyActionIsNotActive = moduleKineticState.caretFakeKeystate.current == false && moduleKineticState.caretFakeKeystate.previous == false;

    if (moduleIsActive && keystateOwnerDiffers && keyActionIsNotActive) {
        // Currently, we share the state among modules & navigation modes, and reset it whenever the user starts to use other mode.
        resetKineticModuleState(&moduleKineticState);

        moduleKineticState.currentModuleId = moduleId;
        moduleKineticState.currentNavigationMode = navigationMode;
    }

    if (moduleKineticState.currentModuleId == moduleId && moduleKineticState.currentNavigationMode == navigationMode) {
        if(moduleConfiguration->invertAxis) {
            float tmp = x;
            x = y;
            y = tmp;
        }

        //we want to process kinetic state even if x == 0 && y == 0, at least as long as caretAxis != CaretAxis_None because of fake key states that may be active.
        processModuleKineticState(x, y, moduleConfiguration, &moduleKineticState);
    }
}

void MouseController_ProcessMouseActions()
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


    if (Slaves[SlaveId_RightTouchpad].isConnected) {
        processTouchpadActions();
        processModuleActions(ModuleId_TouchpadRight, (int16_t)TouchpadEvents.x, (int16_t)TouchpadEvents.y, 0xFF);
        processModuleActions(ModuleId_TouchpadRight, (int16_t)TouchpadEvents.wheelX, (int16_t)TouchpadEvents.wheelY, NavigationMode_Scroll);
        processModuleActions(ModuleId_TouchpadRight, 0, (int16_t)TouchpadEvents.zoomLevel, NavigationMode_Zoom);
        TouchpadEvents.zoomLevel = 0;
        TouchpadEvents.wheelX = 0;
        TouchpadEvents.wheelY = 0;
        TouchpadEvents.x = 0;
        TouchpadEvents.y = 0;
    }

    for (uint8_t moduleSlotId=0; moduleSlotId<UHK_MODULE_MAX_SLOT_COUNT; moduleSlotId++) {
        uhk_module_state_t *moduleState = UhkModuleStates + moduleSlotId;
        if (moduleState->moduleId == ModuleId_Unavailable || moduleState->pointerCount == 0) {
            continue;
        }

        processModuleActions(moduleState->moduleId, (int16_t)moduleState->pointerDelta.x, (int16_t)moduleState->pointerDelta.y, 0xFF);
        moduleState->pointerDelta.x = 0;
        moduleState->pointerDelta.y = 0;
    }

    if (ActiveMouseStates[SerializedMouseAction_LeftClick]) {
        ActiveUsbMouseReport->buttons |= MouseButton_Left;
    }
    if (ActiveMouseStates[SerializedMouseAction_MiddleClick]) {
        ActiveUsbMouseReport->buttons |= MouseButton_Middle;
    }
    if (ActiveMouseStates[SerializedMouseAction_RightClick]) {
        ActiveUsbMouseReport->buttons |= MouseButton_Right;
    }
    if (ActiveMouseStates[SerializedMouseAction_Button_4]) {
        ActiveUsbMouseReport->buttons |= MouseButton_4;
    }
    if (ActiveMouseStates[SerializedMouseAction_Button_5]) {
        ActiveUsbMouseReport->buttons |= MouseButton_5;
    }
    if (ActiveMouseStates[SerializedMouseAction_Button_6]) {
        ActiveUsbMouseReport->buttons |= MouseButton_6;
    }
    if (ActiveMouseStates[SerializedMouseAction_Button_7]) {
        ActiveUsbMouseReport->buttons |= MouseButton_7;
    }
    if (ActiveMouseStates[SerializedMouseAction_Button_8]) {
        ActiveUsbMouseReport->buttons |= MouseButton_8;
    }
}

void ToggleMouseState(serialized_mouse_action_t action, bool activate)
{
    if (activate) {
        ToggledMouseStates[action]++;
        // First macro action is ran during key update cycle, i.e., after ActiveMouseStates is copied from ToggledMouseStates.
        // Otherwise, direction sign will be resetted at the end of this cycle
        ActiveMouseStates[action]++;
        MouseController_ActivateDirectionSigns(action);
    }
    else{
        ToggledMouseStates[action] -= ToggledMouseStates[action] > 0 ? 1 : 0;
    }
}
