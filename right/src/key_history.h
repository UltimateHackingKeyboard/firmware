#ifndef __KEY_HISTORY_H__
#define __KEY_HISTORY_H__

// Includes:

#include "chords.h"
#include "key_states.h"

// Macros:

// Typedefs:

// Variables:

// Functions:

// Registering events
void KeyHistory_RecordPress(const key_state_t *keyState);
void KeyHistory_RecordChordPress(const key_state_t *keyState, const chord_def_t *chord);
void KeyHistory_RecordRelease(const key_state_t *keyState);

// Querying data
bool KeyHistory_WasLastDoubletap();
bool KeyHistory_WasLastMultitap();
uint8_t KeyHistory_GetChordActivationIdOfLastAction();
uint32_t KeyHistory_GetLastActivationTime();


#endif