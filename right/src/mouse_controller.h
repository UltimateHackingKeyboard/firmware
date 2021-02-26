#ifndef __MOUSE_CONTROLLER_H__
#define __MOUSE_CONTROLLER_H__

// Includes:

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

// Variables:

    extern mouse_kinetic_state_t MouseMoveState;
    extern mouse_kinetic_state_t MouseScrollState;

    extern bool ActiveMouseStates[ACTIVE_MOUSE_STATES_COUNT];

// Functions:

    void MouseController_ActivateDirectionSigns(uint8_t state);
    void MouseController_ProcessMouseActions();

#endif
