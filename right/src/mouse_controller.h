#ifndef __MOUSE_CONTROLLER_H__
#define __MOUSE_CONTROLLER_H__

// Includes:
    #include "caret_config.h"
    #include "key_action.h"
    #include "key_states.h"

// Macros:

    #define ABS(A) ((A) < 0 ? (-A) : (A))

// Typedefs:

    typedef struct {
        key_action_cached_t caretAction;
        key_state_t caretFakeKeystate;
        float xFractionRemainder;
        float yFractionRemainder;
        uint32_t lastUpdate;

        uint8_t caretAxis;
        uint8_t currentModuleId;
        uint8_t currentNavigationMode;
        uint8_t zoomPhase;
        uint8_t zoomSign;
        bool zoomActive;
    } module_kinetic_state_t;

// Variables:

    extern uint16_t HoldContinuationTimeout;

// Functions:
    void MouseController_ProcessMouseActions();

#endif
