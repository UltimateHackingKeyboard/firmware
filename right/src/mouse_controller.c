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
        *currentSpeed = distance / elapsedTime;
        lastUpdate = CurrentTime;
    }

    float normalizedSpeed = *currentSpeed/midSpeed;
    return moduleConfiguration->baseSpeed + moduleConfiguration->speed*(float)pow(normalizedSpeed, moduleConfiguration->acceleration);
}

static void processTouchpadActions() {
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

//todo: break this function into parts
void processModuleActions(uint8_t moduleId, float x, float y) {
    module_configuration_t *moduleConfiguration = GetModuleConfiguration(moduleId);
    navigation_mode_t navigationMode = moduleConfiguration->navigationModes[ActiveLayer];
    int16_t yInversion = moduleId == ModuleId_KeyClusterLeft ||  moduleId == ModuleId_TouchpadRight ? -1 : 1;
    static caret_axis_t caretAxis = CaretAxis_None;
    static key_state_t caretFakeKeystate = {};
    static key_action_t* caretAction = &CurrentKeymap[0][0][0];
    int8_t scrollSpeedDivisor = 8;
    float caretSpeedDivisor = 16;
    float caretSkewStrength = 0.5f;

    float speed = computeModuleSpeed(x, y, moduleId);

    static float xFractionRemainder = 0.0f;
    static float yFractionRemainder = 0.0f;
    float xIntegerPart;
    float yIntegerPart;

    if (moduleId == ModuleId_KeyClusterLeft) {
        scrollSpeedDivisor = 1;
        caretSpeedDivisor = 1;
        speed = navigationMode == NavigationMode_Scroll ? 5 : 1;
    }

    switch (navigationMode) {
        case NavigationMode_Cursor: {
            xFractionRemainder = modff(xFractionRemainder + x * speed, &xIntegerPart);
            yFractionRemainder = modff(yFractionRemainder + y * speed, &yIntegerPart);

            ActiveUsbMouseReport->x += xIntegerPart;
            ActiveUsbMouseReport->y -= yInversion*yIntegerPart;

            break;
        }
        case NavigationMode_Scroll: {
            if (moduleId == ModuleId_KeyClusterLeft && (x != 0 || y != 0)) {
                xFractionRemainder = 0;
                yFractionRemainder = 0;
            }

            xFractionRemainder = modff(xFractionRemainder + x * speed / scrollSpeedDivisor, &xIntegerPart);
            yFractionRemainder = modff(yFractionRemainder + y * speed / scrollSpeedDivisor, &yIntegerPart);

            ActiveUsbMouseReport->wheelX += xIntegerPart;
            ActiveUsbMouseReport->wheelY += yInversion*yIntegerPart;
            break;
        }
        case NavigationMode_Media:
        case NavigationMode_Caret: {
            //optimize this out if nothing is going on
            if(x == 0 && y == 0 && caretAxis == CaretAxis_None) {
                break;
            }
            caret_configuration_t* currentCaretConfig = GetModuleCaretConfiguration(moduleId, navigationMode);

            //unlock axis if inactive for some time and re-activate tick trashold`
            if (x != 0 || y != 0) {
                static uint16_t lastUpdate = 0;

                if(CurrentTime - lastUpdate > 500 && caretAxis != CaretAxis_None) {
                    xFractionRemainder = 0;
                    yFractionRemainder = 0;
                    caretAxis = CaretAxis_None;
                }
                lastUpdate = CurrentTime;
            }

            // caretAxis tries to lock to one direction, therefore we "skew" the other one
            float caretXModeMultiplier = caretAxis == CaretAxis_Horizontal ? 1.0f : caretSkewStrength;
            float caretYModeMultiplier = caretAxis == CaretAxis_Vertical ? 1.0f : caretSkewStrength;

            xFractionRemainder += x * speed / caretSpeedDivisor * caretXModeMultiplier;
            yFractionRemainder += y * speed / caretSpeedDivisor * caretYModeMultiplier;


            //If there is an ongoing action, just handle that action via a fake state. Ensure that full lifecycle of a key gets executed.
            if(caretFakeKeystate.current || caretFakeKeystate.previous) {
                bool tmp = caretFakeKeystate.current;
                caretFakeKeystate.current = !caretFakeKeystate.previous;
                caretFakeKeystate.previous = tmp;
                ApplyKeyAction(&caretFakeKeystate, caretAction, caretAction);
            }
            //If we want to start a new action (new "tick")
            else {
                // determine current axis properties and setup indirections for easier handling
                caret_axis_t axisCandidate = caretAxis == CaretAxis_Inactive ? CaretAxis_Vertical : caretAxis;
                float* axisFractionRemainders [CaretAxis_Count] = {&xFractionRemainder, &yFractionRemainder};
                float axisIntegerParts [CaretAxis_Count] = { 0, 0 };

                modff(xFractionRemainder, &axisIntegerParts[CaretAxis_Horizontal]);
                modff(yFractionRemainder, &axisIntegerParts[CaretAxis_Vertical]);

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
                    caretAxis = axisCandidate;
                    float sgn = axisIntegerParts[axisCandidate] > 0 ? 1 : -1;
                    *axisFractionRemainders[1 - axisCandidate] = 0.0f;
                    *axisFractionRemainders[axisCandidate] -= sgn;
                    caret_dir_action_t* dirActions = &currentCaretConfig->axisActions[caretAxis];
                    caretAction = sgn*yInversion > 0 ? &dirActions->positiveAction : &dirActions->negativeAction;
                    caretFakeKeystate.current = true;
                    ApplyKeyAction(&caretFakeKeystate, caretAction, caretAction);
                }
            }
            break;
        }
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
        processModuleActions(ModuleId_TouchpadRight, (int16_t)TouchpadEvents.x, (int16_t)TouchpadEvents.y);
        TouchpadEvents.x = 0;
        TouchpadEvents.y = 0;
    }

    for (uint8_t moduleSlotId=0; moduleSlotId<UHK_MODULE_MAX_SLOT_COUNT; moduleSlotId++) {
        uhk_module_state_t *moduleState = UhkModuleStates + moduleSlotId;
        if (moduleState->moduleId == ModuleId_Unavailable || moduleState->pointerCount == 0) {
            continue;
        }

        processModuleActions(moduleState->moduleId, (int16_t)moduleState->pointerDelta.x, (int16_t)moduleState->pointerDelta.y);
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
