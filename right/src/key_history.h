#ifndef __KEY_HISTORY_H__
#define __KEY_HISTORY_H__

// Includes:

#include "key_states.h"

// Macros:

// Typedefs:

typedef struct {
    const key_state_t *keyState;
    uint32_t timestamp;
    uint8_t multiTapCount : 6;
    uint8_t keyActivationId : 4;
    bool multiTapBreaker : 1;
} key_press_event_t;

// Variables:

// Functions:

// Recording events
void KeyHistory_RecordPress(const key_state_t *keyState);
void KeyHistory_RecordRelease(const key_state_t *keyState);

// Querying whole events
const key_press_event_t * KeyHistory_GetPreceedingPress(const key_state_t *keyState, uint8_t activationId);

// Querying specific info
uint8_t KeyHistory_GetMultitapCount(const key_state_t *keyState, uint8_t activationId);

#endif