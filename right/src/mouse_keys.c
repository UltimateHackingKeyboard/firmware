#include <math.h>
#include "key_action.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "timer.h"
#include "config_parser/parse_keymap.h"
#include "mouse_keys.h"
#include "debug.h"
#include "config_manager.h"
#include "event_scheduler.h"
#include <string.h>
#include <assert.h>

static uint32_t mouseUsbReportUpdateTime = 0;
static uint32_t mouseElapsedTime;

uint8_t ActiveMouseStates[ACTIVE_MOUSE_STATES_COUNT];
uint8_t ToggledMouseStates[ACTIVE_MOUSE_STATES_COUNT];


usb_mouse_report_t MouseKeysMouseReport;

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
void MouseKeys_ActivateDirectionSigns(uint8_t state) {
    switch (state) {
    case SerializedMouseAction_MoveUp:
        Cfg.MouseMoveState.verticalStateSign = -1;
        break;
    case SerializedMouseAction_MoveDown:
        Cfg.MouseMoveState.verticalStateSign = 1;
        break;
    case SerializedMouseAction_MoveLeft:
        Cfg.MouseMoveState.horizontalStateSign = -1;
        break;
    case SerializedMouseAction_MoveRight:
        Cfg.MouseMoveState.horizontalStateSign = 1;
        break;
    case SerializedMouseAction_ScrollUp:
        Cfg.MouseScrollState.verticalStateSign = 1;
        break;
    case SerializedMouseAction_ScrollDown:
        Cfg.MouseScrollState.verticalStateSign = -1;
        break;
    case SerializedMouseAction_ScrollLeft:
        Cfg.MouseScrollState.horizontalStateSign = -1;
        break;
    case SerializedMouseAction_ScrollRight:
        Cfg.MouseScrollState.horizontalStateSign = 1;
        break;
    }
}

static void processMouseKineticState(mouse_kinetic_state_t *kineticState)
{
    float scrollMultiplier = 1.f;
    if (kineticState->isScroll) {
        // in practice the vertical and horizontal scroll multipliers are always the same
        scrollMultiplier = VerticalScrollMultiplier();
    }
    float initialSpeed = scrollMultiplier * kineticState->intMultiplier * kineticState->initialSpeed;
    float acceleration = scrollMultiplier * kineticState->intMultiplier * kineticState->acceleration;
    float deceleratedSpeed = scrollMultiplier * kineticState->intMultiplier * kineticState->deceleratedSpeed;
    float baseSpeed = scrollMultiplier * kineticState->intMultiplier * kineticState->baseSpeed;
    float acceleratedSpeed = scrollMultiplier * kineticState->intMultiplier * kineticState->acceleratedSpeed;

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

        if ( kineticState->horizontalStateSign != 0 && kineticState->verticalStateSign != 0 && Cfg.DiagonalSpeedCompensation ) {
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

void MouseKeys_ProcessMouseActions()
{
    EventVector_Unset(EventVector_MouseKeys);
    mouseElapsedTime = Timer_GetElapsedTimeAndSetCurrent(&mouseUsbReportUpdateTime);
    memset(&MouseKeysMouseReport, 0, sizeof(MouseKeysMouseReport));

    processMouseKineticState(&Cfg.MouseMoveState);
    MouseKeysMouseReport.x += Cfg.MouseMoveState.xOut;
    MouseKeysMouseReport.y += Cfg.MouseMoveState.yOut;
    Cfg.MouseMoveState.xOut = 0;
    Cfg.MouseMoveState.yOut = 0;

    processMouseKineticState(&Cfg.MouseScrollState);
    MouseKeysMouseReport.wheelX += Cfg.MouseScrollState.xOut;
    MouseKeysMouseReport.wheelY += Cfg.MouseScrollState.yOut;
    Cfg.MouseScrollState.xOut = 0;
    Cfg.MouseScrollState.yOut = 0;

    if (ActiveMouseStates[SerializedMouseAction_LeftClick]) {
        MouseKeysMouseReport.buttons |= MouseButton_Left;
    }
    if (ActiveMouseStates[SerializedMouseAction_MiddleClick]) {
        MouseKeysMouseReport.buttons |= MouseButton_Middle;
    }
    if (ActiveMouseStates[SerializedMouseAction_RightClick]) {
        MouseKeysMouseReport.buttons |= MouseButton_Right;
    }
    for (uint8_t serializedButton = SerializedMouseAction_Button_4; serializedButton <= SerializedMouseAction_Button_Last; serializedButton++)
    {
        if (ActiveMouseStates[serializedButton]) {
            MouseKeysMouseReport.buttons |= 1 << ((serializedButton - SerializedMouseAction_Button_4) + 3);
        }
    }

    bool wasMoving = Cfg.MouseMoveState.wasMoveAction || Cfg.MouseScrollState.wasMoveAction;
    if (MouseKeysMouseReport.buttons || wasMoving) {
        EventVector_Set(EventVector_MouseKeysReportsUsed);
    }
    if (wasMoving) {
        EventVector_Set(EventVector_MouseKeys | EventVector_SendUsbReports);
    }
}

void MouseKeys_SetState(serialized_mouse_action_t action, bool lock, bool activate)
{
    EventVector_Set(EventVector_MouseKeys | EventVector_SendUsbReports);
    if (activate) {
        if (lock) {
            EventVector_Set(EventVector_NativeActions);
            ToggledMouseStates[action]++;
            // First macro action is ran during key update cycle, i.e., after ActiveMouseStates is copied from ToggledMouseStates.
            // Otherwise, direction sign will be resetted at the end of this cycle
            ActiveMouseStates[action]++;
        }
        MouseKeys_ActivateDirectionSigns(action);
        mouseUsbReportUpdateTime = CurrentTime;
    }
    else{
        if (lock) {
            EventVector_Set(EventVector_NativeActions);
            ToggledMouseStates[action] -= ToggledMouseStates[action] > 0 ? 1 : 0;
        }
    }
}
