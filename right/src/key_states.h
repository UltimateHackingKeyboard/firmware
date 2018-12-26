#ifndef __KEY_STATES_H__
#define __KEY_STATES_H__

// Includes:

    #include "fsl_common.h"
    #include "slot.h"
    #include "module.h"

// Typedefs:

    typedef struct {
        uint8_t timestamp;
        bool previous : 1;
        bool current : 1;
        bool debouncing : 1;
    } key_state_t;

// Variables:

    extern key_state_t KeyStates[SLOT_COUNT][MAX_KEY_COUNT_PER_MODULE];

#endif
