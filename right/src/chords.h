#ifndef __CHORDS_H__
#define __CHORDS_H__

#include "key_action.h"

#define MAX_CHORD_KEYS 5
typedef uint8_t chord_keys_t[MAX_CHORD_KEYS];

typedef enum {
    ChordSearch_Nothing = 0b00, // There is no chord which could match provided data
    ChordSearch_Partial = 0b01, // There was no exact match, but there were chords eligible with more keys
    ChordSearch_Exact = 0b10, // There was an exact match and no other potential matches
    ChordSearch_ExactAndPartial = 0b11, // There was a match, but there are potentially longer chords
} chord_search_result_t;

typedef struct {
    layer_id_t layer;
    uint8_t keyCount;
    chord_keys_t keys;

    key_action_t action;
} ATTR_PACKED chord_def_t;

bool Chords_TryAddChord(layer_id_t layer, chord_keys_t keys, uint8_t keyCount, key_action_t *action);
void Chords_ResetChords();

typedef enum {
    ChordResolution_Wait,
    ChordResolution_Failed,
    ChordResolution_Resolved,
} chord_resolution_t;

chord_resolution_t Chords_Driver(key_state_t *keyState, layer_id_t layer, chord_def_t **out_matchedChord);
 
#endif
