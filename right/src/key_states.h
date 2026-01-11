#ifndef __KEY_STATES_H__
#define __KEY_STATES_H__

// Includes:

#ifndef __ZEPHYR__
    #include "fsl_common.h"
#endif
    #include "slot.h"
    #include "module.h"

// Typedefs:

    typedef enum {
        SecondaryRoleState_DontKnowYet,
        SecondaryRoleState_Primary,
        SecondaryRoleState_Secondary,
        SecondaryRoleState_NoOp,
    } secondary_role_state_t;

    // Next is used as an accumulator of the state - asynchronous state updates
    // are stored there. The hardwareSwitchState always contains the most up-to-date
    // information about hardware state of the switch.
    //
    // `Previous` and `current` are computed from `hardwareSwitchState` by "debouncing"
    // algorithm.  Especially values (0, 1) signify that key has been pressed
    // right now and an action (e.g., start of a macro) should take place.
    //
    // Debouncing flag & timestamp are used by debouncer to prevent the value
    // of current from changing for next 50 ms whenever the key state changes.

    typedef struct {
        uint8_t timestamp;
        volatile bool hardwareSwitchState : 1;
        bool debouncedSwitchState : 1;
        bool current : 1;
        bool previous : 1;
        bool debouncing : 1;
        secondary_role_state_t secondaryState : 2;
    } key_state_t;

// Variables:

    extern key_state_t KeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

// Inline functions

    static inline bool KeyState_Active(key_state_t* s) { return s->current; };
    static inline bool KeyState_Inactive(key_state_t* s) { return !s->current; };
    static inline bool KeyState_ActivatedNow(key_state_t* s) { return !s->previous && s->current; };
    static inline bool KeyState_DeactivatedNow(key_state_t* s) { return s->previous && !s->current; };
    static inline bool KeyState_ActivatedEarlier(key_state_t* s) { return s->previous && s->current; };
    static inline bool KeyState_DeactivatedEarlier(key_state_t* s) { return !s->previous && !s->current; };
    static inline bool KeyState_NonZero(key_state_t* s) { return s->previous || s->current; };

    static inline bool KeyState_IsRightHalf(key_state_t* s) { return s < &KeyStates[1][0]; };
    static inline bool KeyState_IsLeftHalf(key_state_t* s) { return s < &KeyStates[2][0] && s >= &KeyStates[1][0]; };
    static inline bool KeyState_IsKeyCluster(key_state_t* s) { return s >= &KeyStates[2][0] && s < &KeyStates[3][0]; };
    static inline bool KeyState_IsMouseModule(key_state_t* s) { return s >= &KeyStates[3][0]; };
    static inline bool KeyState_IsModule(key_state_t* s) { return s >= &KeyStates[2][0]; };
    static inline bool KeyState_IsRightSide(key_state_t* s) { return KeyState_IsRightHalf(s) || KeyState_IsMouseModule(s); };
    static inline bool KeyState_IsLeftSide(key_state_t* s) { return s >= &KeyStates[1][0] && s < &KeyStates[3][0]; };


#endif
