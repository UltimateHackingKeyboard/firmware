#ifndef __KEY_HISTORY_H__
#define __KEY_HISTORY_H__

// Includes:

#include "key_states.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

void KeyHistory_RecordPress(const key_state_t *keyState);
void KeyHistory_RecordRelease(const key_state_t *keyState);
bool KeyHistory_IsDoubletap(const key_state_t *keyState, const uint16_t doubletapInterval);

#endif