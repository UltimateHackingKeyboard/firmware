#ifndef __KEY_STATES_H__
#define __KEY_STATES_H__

// Includes:

    #include "fsl_common.h"
    #include "slot.h"
    #include "module.h"

// Typedefs:

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
        bool secondary : 1;
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

#endif
