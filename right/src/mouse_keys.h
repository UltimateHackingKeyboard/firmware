#ifndef __MOUSE_KEYS_H__
#define __MOUSE_KEYS_H__

// Includes:

    #include "usb_interfaces/usb_interface_mouse.h"
    #include "caret_config.h"
    #include "key_action.h"
    #include "key_states.h"

// Macros:

    #define ACTIVE_MOUSE_STATES_COUNT (SerializedMouseAction_Last + 1)
    #define ABS(A) ((A) < 0 ? (-A) : (A))

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
        float axisSkew;
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

    extern uint8_t ActiveMouseStates[ACTIVE_MOUSE_STATES_COUNT];
    extern uint8_t ToggledMouseStates[ACTIVE_MOUSE_STATES_COUNT];
    extern usb_mouse_report_t MouseKeysMouseReport;


// Functions:

    void MouseKeys_ActivateDirectionSigns(uint8_t state);
    void MouseKeys_ProcessMouseActions();
    void MouseKeys_SetState(serialized_mouse_action_t action, bool lock, bool activate);

#endif
