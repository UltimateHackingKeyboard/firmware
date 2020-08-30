#include <math.h>
#include "key_action.h"
#include "led_display.h"
#include "layer.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "peripherals/test_led.h"
#include "slave_drivers/is31fl37xx_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "timer.h"
#include "config_parser/parse_keymap.h"
#include "usb_commands/usb_command_get_debug_buffer.h"
#include "arduino_hid/ConsumerAPI.h"
#include "secondary_role_driver.h"
#include "slave_drivers/touchpad_driver.h"
#include "mouse_controller.h"
#include "slave_scheduler.h"

static uint32_t mouseUsbReportUpdateTime = 0;
static uint32_t mouseElapsedTime;

bool ActiveMouseStates[ACTIVE_MOUSE_STATES_COUNT];

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

    bool isMoveAction = ActiveMouseStates[kineticState->upState] ||
                        ActiveMouseStates[kineticState->downState] ||
                        ActiveMouseStates[kineticState->leftState] ||
                        ActiveMouseStates[kineticState->rightState];

    mouse_speed_t mouseSpeed = MouseSpeed_Normal;
    if (ActiveMouseStates[SerializedMouseAction_Accelerate]) {
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

uint8_t touchpadScrollDivisor = 8;
static void processTouchpadActions() {
    ActiveUsbMouseReport->x += TouchpadEvents.x;
    ActiveUsbMouseReport->y += TouchpadEvents.y;
    TouchpadEvents.x = 0;
    TouchpadEvents.y = 0;

    uint8_t wheelXInteger = TouchpadEvents.wheelX / touchpadScrollDivisor;
    if (wheelXInteger) {
        ActiveUsbMouseReport->wheelX += wheelXInteger;
        TouchpadEvents.wheelX = TouchpadEvents.wheelX % touchpadScrollDivisor;
    }

    uint8_t wheelYInteger = TouchpadEvents.wheelY / touchpadScrollDivisor;
    if (wheelYInteger) {
        ActiveUsbMouseReport->wheelY -= wheelYInteger;
        TouchpadEvents.wheelY = TouchpadEvents.wheelY % touchpadScrollDivisor;
    }


    if (TouchpadEvents.singleTap) {
        ActiveUsbMouseReport->buttons |= MouseButton_Left;
        TouchpadEvents.singleTap = false;
    }

    if (TouchpadEvents.twoFingerTap) {
        ActiveUsbMouseReport->buttons |= MouseButton_Right;
        TouchpadEvents.twoFingerTap = false;
    }

    if (TouchpadEvents.tapAndHold) {
        ActiveUsbMouseReport->buttons |= MouseButton_Left;
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
    }

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
