#ifndef __KEY_STATES_H__
#define __KEY_STATES_H__

// Includes:

#ifndef __ZEPHYR__
    #include "fsl_common.h"
#endif
    #include "slot.h"
    #include "module.h"

// Macros:

    #define KEYSTATE_KEYINACTIVE(keyState) ((uint8_t*)keyState)[1] == 0
    
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
        uint8_t debounceTimestamp;
        // Remember to update KEY_INACTIVE() if adding or removing stuff before the bitfield
        volatile bool hardwareSwitchState : 1;
        bool debouncedSwitchState : 1;
        bool current : 1;
        bool previous : 1;
        bool debouncing : 1;
        secondary_role_state_t secondaryState : 2;
        bool padding : 1; // This allows the KEY_INACTIVE() macro to not trigger false because of sequence
        uint8_t activationId: 3;
    } key_state_t;

// Variables:

    extern key_state_t KeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

// Inline functions

    static inline bool KeyState_Active(const key_state_t* s) { return s->current; };
    static inline bool KeyState_Inactive(const key_state_t* s) { return !s->current; };
    static inline bool KeyState_ActivatedNow(const key_state_t* s) { return !s->previous && s->current; };
    static inline bool KeyState_DeactivatedNow(const key_state_t* s) { return s->previous && !s->current; };
    static inline bool KeyState_ActivatedEarlier(const key_state_t* s) { return s->previous && s->current; };
    static inline bool KeyState_DeactivatedEarlier(const key_state_t* s) { return !s->previous && !s->current; };
    static inline bool KeyState_NonZero(const key_state_t* s) { return s->previous || s->current; };

    static inline bool KeyState_IsRightHalf(const key_state_t* s) { return s < &KeyStates[1][0]; };
    static inline bool KeyState_IsLeftHalf(const key_state_t* s) { return s < &KeyStates[2][0] && s >= &KeyStates[1][0]; };
    static inline bool KeyState_IsKeyCluster(const key_state_t* s) { return s >= &KeyStates[2][0] && s < &KeyStates[3][0]; };
    static inline bool KeyState_IsMouseModule(const key_state_t* s) { return s >= &KeyStates[3][0]; };
    static inline bool KeyState_IsModule(const key_state_t* s) { return s >= &KeyStates[2][0]; };
    static inline bool KeyState_IsRightSide(const key_state_t* s) { return KeyState_IsRightHalf(s) || KeyState_IsMouseModule(s); };
    static inline bool KeyState_IsLeftSide(const key_state_t* s) { return s >= &KeyStates[1][0] && s < &KeyStates[3][0]; };


#endif
