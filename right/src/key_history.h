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
bool KeyHistory_WasLastDoubletap();
void KeyHistory_ConsumeDoubletap(const key_state_t *keyState, uint8_t keyActivationId);

#endif