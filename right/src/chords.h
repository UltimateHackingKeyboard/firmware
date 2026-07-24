#ifndef __CHORDS_H__
#define __CHORDS_H__

#include "key_action.h"

#define MAX_CHORD_KEYS 5
#define CHORDS_ACTIVATIONID_SIZE 6
#define CHORDS_INVALID_ACTIVATION_ID 0

typedef enum {
    ChordApplicationType_LeadingKey,
    ChordApplicationType_AllKeys,
} chord_application_type_t;

typedef uint8_t chord_keys_t[MAX_CHORD_KEYS];

typedef struct {
    // Chord definition
    layer_id_t layer;
    chord_keys_t keys;
    key_action_t action;
    uint8_t keyCount : 3;

    // Chord state
    uint8_t pressedStates : MAX_CHORD_KEYS;
    uint8_t activationId : CHORDS_ACTIVATIONID_SIZE;
} ATTR_PACKED chord_def_t;

bool Chords_TryAddChord(layer_id_t layer, chord_keys_t keys, uint8_t keyCount, key_action_t *action);
void Chords_ResetChords();

typedef enum {
    ChordResolution_Wait,
    ChordResolution_Failed,
    ChordResolution_Resolved,
} chord_resolution_t;

chord_resolution_t Chords_Driver(key_state_t *keyState, layer_id_t layer, const chord_def_t **out_matchedChord);
void Chords_KeyReleased(const key_state_t *keyState);
bool Chords_IsChordActivationActive(uint8_t activationId, bool checkPostponer); 

#endif
