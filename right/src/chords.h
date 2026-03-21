#ifndef __CHORDS_H__
#define __CHORDS_H__

#include "key_action.h"

#define MAX_CHORD_KEYS 5
typedef uint8_t chord_keys_t[MAX_CHORD_KEYS];

typedef enum {
    ChordSearch_Nothing, // There is no chord which could match provided data
    ChordSearch_PartialOnly, // There are no finished chords, but some partially finished
    ChordSearch_MatchAndPartial, // There was an exact match, but also longer chords
    ChordSearch_FinalMatch, // There was an exact match and no other potential matches
} chord_search_result_t;

typedef struct {
    chord_keys_t keys;

    key_action_t action;

    uint8_t layer : 4;

    uint8_t keyCount : 3;
} ATTR_PACKED chord_def_t;

bool Chords_TryAddChord(uint8_t layer, chord_keys_t keys, uint8_t keyCount, key_action_t *action);
chord_search_result_t Chords_TryGetChordAction(key_action_t *outAction, uint8_t layer, chord_keys_t keys, uint8_t keyCount);
void Chords_ResetChords();
 
#endif
