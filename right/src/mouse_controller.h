#ifndef __MOUSE_CONTROLLER_H__
#define __MOUSE_CONTROLLER_H__

// Includes:
    #include "caret_config.h"
    #include "key_action.h"
    #include "key_states.h"

// Macros:

    #define ACTIVE_MOUSE_STATES_COUNT (SerializedMouseAction_Last + 1)

// Typedefs:

    typedef enum {
        MouseSpeed_Normal,
        MouseSpeed_Accelerated,
        MouseSpeed_Decelerated,
    } mouse_speed_t;

    typedef struct {
        bool isScroll;
        bool wasMoveAction;
        serialized_mouse_action_t upState;
        serialized_mouse_action_t downState;
        serialized_mouse_action_t leftState;
        serialized_mouse_action_t rightState;
        mouse_speed_t prevMouseSpeed;
        float intMultiplier;
        float currentSpeed;
        float targetSpeed;
        uint8_t initialSpeed;
        uint8_t acceleration;
        uint8_t deceleratedSpeed;
        uint8_t baseSpeed;
        uint8_t acceleratedSpeed;
        float xSum;
        float ySum;
        int16_t xOut;
        int16_t yOut;
        int8_t verticalStateSign;
        int8_t horizontalStateSign;
    } mouse_kinetic_state_t;

    typedef struct {
        key_action_t* caretAction;
        key_state_t caretFakeKeystate;
        float xFractionRemainder;
        float yFractionRemainder;
        uint32_t lastUpdate;

        uint8_t caretAxis;
        uint8_t currentModuleId;
        uint8_t currentNavigationMode;
    } module_kinetic_state_t;

// Variables:

    extern mouse_kinetic_state_t MouseMoveState;
    extern mouse_kinetic_state_t MouseScrollState;

    extern uint8_t ActiveMouseStates[ACTIVE_MOUSE_STATES_COUNT];
    extern uint8_t ToggledMouseStates[ACTIVE_MOUSE_STATES_COUNT];

    extern bool CompensateDiagonalSpeed;

// Functions:
    void MouseController_ActivateDirectionSigns(uint8_t state);
    void MouseController_ProcessMouseActions();

#endif
