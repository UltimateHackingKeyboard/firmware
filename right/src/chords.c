#include "chords.h"
#include "macros/status_buffer.h"

#define MAX_CHORDS_COUNT 64

chord_def_t Chords[MAX_CHORDS_COUNT];
uint8_t ChordCount = 0;

static inline void sortKeys(chord_keys_t keys, uint8_t keyCount)
{
    // Some kind of bubblesort, I dunno
    bool recheck = true;
    while ( recheck ) {
        recheck = false;
        for (int i = 0; i < keyCount - 1; ++i) {
            if (keys[i] > keys[i + 1] ) {
                uint8_t temp = keys[i];
                keys[i] = keys[i + 1];
                keys[i + 1] = temp;
                if (i < keyCount - 1) {
                    // If the keys we just switched were not the last keys, we need to go again.
                    recheck = true;
                }
            }
        }
    }
}

/*
Chords are stored sorted as follows:
First by layer, direction doesn't matter, to group by layer
Then ascending by key count - ascending, this matters
Then by key id list, direction doesn't matter, only that they're sorted consistently
*/
static inline int8_t compareToChord(const chord_def_t *left, uint8_t layer, uint8_t keyCount, const chord_keys_t keys) {
    // These could be collapsed and optimized more if we're willing to do union with a bitfield, but that's hacky
    if (left->layer != layer) {
        return left->layer - layer;
    }
    if (left->keyCount != keyCount) {
        return left->keyCount - keyCount;
    }
    return memcmp(left->keys, keys, keyCount);
}

bool Chords_TryAddChord(uint8_t layer, chord_keys_t keys, uint8_t keyCount, key_action_t *action)
{
    sortKeys(keys, keyCount);

    // Find a spot
    uint8_t i = 0;
    int8_t chordCmp = 1; // Initialize to not zero to not overwrite first on empty list
    while (i < ChordCount && (chordCmp = compareToChord(&Chords[i], layer, keyCount, keys)) < 0) {++i;}

    if (chordCmp == 0) {
        // We already have the chord, erase or update, depending on provided action
        if (action->type == KeyActionType_None) {
            // Erase the chord and return
            memmove(&Chords[i], &Chords[i + 1], sizeof(chord_def_t) * (--ChordCount - i));
            memset(&Chords[ChordCount], 0, sizeof(chord_def_t));
            return true;
        }
        // Just overwrite action, the rest of the chord is already in place
        Chords[i].action = *action;
        return true;
    }

    if (action->type == KeyActionType_None) {
        // We were told to remove it, but we don't have it, great success!
        return true;
    }

    // Check if there is room if we're not removing a chord
    if (action->type != KeyActionType_None && ChordCount >= MAX_CHORDS_COUNT) {
        // No room
        return false;
    }

    // We do not have the chord, but we know where to put it now
    // Insert the chord.
    memmove(&Chords[i + 1], &Chords[i], sizeof(chord_def_t) * (ChordCount++ - i));
    chord_def_t *chord = &Chords[i];
    memcpy(chord->keys, keys, MAX_CHORD_KEYS);
    chord->keyCount = keyCount;
    chord->layer = layer;
    chord->action = *action;
    return true;
}

// This function assumes that both lists of keys are sorted smallest to largest.
static bool isPartialOfChord(const chord_def_t *large, chord_keys_t keys, uint8_t keyCount)
{
    uint8_t i = 0, j = 0;
    for (; i < keyCount; ++i) {
        // We can skip keys in the chord to see if all the provided keys exist
        while (keys[i] > large->keys[j]) {
            ++j;
        }
        if (keys[i] != large->keys[j]) {
            // A provided key was not in the chord
            // The provided key cannot grow into the chord
            return false;
        }
        ++j;
    }
    return true;
}

chord_search_result_t Chords_TryGetChordAction(key_action_t *outAction, uint8_t layer, chord_keys_t keys, uint8_t keyCount)
{
    sortKeys(keys, keyCount);

    uint8_t i = 0;
    bool matched = false;
    bool partial = false;

    // First maybe find the chord
    for (; i < ChordCount; ++i) {
        int8_t chordCmp = compareToChord(&Chords[i], layer, keyCount, keys);
        if (chordCmp < 0) continue;
        if (chordCmp == 0) {
            // We found the longest completed chord so far
            *outAction = Chords[i].action;
            matched = true;
        }
        break;
    }
    
    // We now may have a match, but we can't return it yet.
    // We need to check if we have a potential for a longer chord
    // Because of the sort order of the chord list, all chords after this slot
    // with the same layer are same length or longer than the desired chord.
    
    // Now iterate until we get to the longer chords in the layer because
    // we want to see if there are potential matches in them
    while (i < ChordCount && Chords[i].layer == layer && Chords[i].keyCount == keyCount) {++i;}

    // i is now at the first longer chord in the layer, or the first key in another layer, on at list's end
    // Now we look for potential matches with more keys
    while (i < ChordCount && Chords[i].layer == layer) {
        if (isPartialOfChord(&Chords[i++], keys, keyCount)) {
            // If we have a partial chord
            partial = true;
            break;
        }
    }

    if (matched) {
        // We have found an exact match
        return partial ? ChordSearch_MatchAndPartial : ChordSearch_FinalMatch;
    }
    if (partial) {
        return ChordSearch_PartialOnly;
    }
    return ChordSearch_Nothing;
}

void Chords_ResetChords() {
    ChordCount = 0;
}