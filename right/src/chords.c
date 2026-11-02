#include "chords.h"
#include "config_manager.h"
#include "event_scheduler.h"
#include "key_history.h"
#include "keymap.h"
#include "layer.h"
#include "macros/status_buffer.h"
#include "postponer.h"
#include "utils.h"

#define MAX_CHORDS_COUNT 64

// Not sure if this should be an enum or just macros.
typedef enum {
    ChordSearch_Partial = 0b01, // There was no exact match, but there were chords eligible with more keys
    ChordSearch_Exact = 0b10, // There was an exact match and no other potential matches
} chord_search_result_t;

chord_def_t Chords[MAX_CHORDS_COUNT];
uint8_t ChordCount = 0;
uint8_t NextActivationId = CHORDS_INVALID_ACTIVATION_ID + 1;

// Not sure how nice or not this is.  I like named parameters rather than just true/false in calls.
// I also don't like the all-caps of macros, this C doesn't have consexpr, and I don't know if global consts will get optimized.
typedef enum {
    KeyReleased = false,
    KeyPressed = true,
} key_pressed_state_t;

static struct {
    const key_state_t *initialKey;
    uint32_t pressTime;
    uint32_t releaseTime;
    uint8_t unrollingKeysCount;
    chord_def_t *unrollingChord;
} ResolutionState;

static inline void sortKeys(chord_keys_t keys, uint8_t keyCount)
{
    // Some kind of bubblesort, I dunno, 5 keys max, don't care about optimal algorithm
    bool recheck = true;
    while ( recheck ) {
        recheck = false;
        for (uint8_t i = 0; i < keyCount - 1; ++i) {
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

uint8_t findUsedKeys(chord_keys_t unusedKeys, uint8_t keyCount, const chord_def_t *chord) {
    uint8_t i = 0;
    uint8_t j = 0;
    while (i < keyCount && j < chord->keyCount) {
        while (i < keyCount && unusedKeys[i] < chord->keys[j]) {++i;}
        if (unusedKeys[i] == chord->keys[j]) {
            memmove(unusedKeys + i, unusedKeys + i + 1, keyCount-- - i);
        }
        while (j < chord->keyCount && chord->keys[j] < unusedKeys[i]) {++j;}
    }
    return keyCount;
}

void handleUpdateKeysOfDeletedChordOnLayer(layer_id_t layer, chord_keys_t keys, uint8_t keyCount) {
    for (uint8_t i = 0; i < ChordCount; ++i) {
        if (Chords[i].layer < layer) continue;
        if (Chords[i].layer > layer && Chords[i].layer != LayerId_None) continue;
        if ((keyCount = findUsedKeys(keys, keyCount, &Chords[i])) == 0) break;
    }

    for (uint8_t i = 0; i < keyCount; ++i) {
        CurrentKeymap[layer][keys[i]/64][keys[i]%64].isPartOfChord = false;
    }
}

void handleUpdateKeysOfDeletedChord(layer_id_t layer, chord_keys_t keys, uint8_t keyCount) {
    if (layer == LayerId_None) {
        for (layer_id_t loopLayer = LayerId_Base; loopLayer < LayerId_Count; ++loopLayer) {
            handleUpdateKeysOfDeletedChordOnLayer(loopLayer, keys, keyCount);
        }
    }
    else {
        handleUpdateKeysOfDeletedChordOnLayer(layer, keys, keyCount);
    }
}

void markKeysAsPartsOfChord(chord_def_t *chord) {
    for (uint8_t i = 0; i < chord->keyCount; ++i) {
        if (chord->layer != LayerId_None) {
            CurrentKeymap[chord->layer][chord->keys[i]/64][chord->keys[i]%64].isPartOfChord = true;
        }
        else {
            for (layer_id_t j = 0; j < LayerId_Count; ++j) {
                CurrentKeymap[j][chord->keys[i]/64][chord->keys[i]%64].isPartOfChord = true;
            }
        }
    }
}


/*
Chords are stored sorted as follows:
First by layer, ascending to put the any layer (255) last
Then ascending by key count - ascending so we know that chord size within a layer's chords increases
Then by key id list, direction doesn't matter, only that they're sorted consistently
*/
static inline int16_t compareToChord(const chord_def_t *left, layer_id_t layer, uint8_t keyCount, const chord_keys_t keys) {
    // These could be collapsed and optimized more if we're willing to do union with a bitfield, but that's hacky
    if (left->layer != layer) {
        return (int16_t)left->layer - layer;
    }
    if (left->keyCount != keyCount) {
        return (int16_t)left->keyCount - keyCount;
    }
    return memcmp(left->keys, keys, keyCount);
}

/*
Search here is implemented as just iteration over the list.
One could do a binary search instead, for a minor speed increase.
I might do that later.
*/
bool Chords_TryAddChord(layer_id_t layer, chord_keys_t keys, uint8_t keyCount, key_action_t *action)
{
    sortKeys(keys, keyCount);

    // Find a spot
    uint8_t i = 0;
    int16_t chordCmp = 1; // Initialize to not zero to not overwrite first on empty list
    while (i < ChordCount && (chordCmp = compareToChord(&Chords[i], layer, keyCount, keys)) < 0) {++i;}

    if (chordCmp == 0) {
        // We already have the chord, erase or update, depending on provided action
        if (action->type == KeyActionType_None) {
            // Erase the chord
            memmove(&Chords[i], &Chords[i + 1], sizeof(chord_def_t) * (--ChordCount - i));
            handleUpdateKeysOfDeletedChord(layer, keys, keyCount);
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
    markKeysAsPartsOfChord(chord);
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

// Again linear search, not binary
chord_search_result_t searchForChordAction(chord_def_t **out_matchedChord, layer_id_t layer, chord_keys_t keys, uint8_t keyCount, bool noPartial)
{
    sortKeys(keys, keyCount);

    uint8_t i = 0;
    bool matched = false;
    bool partialMatched = false;
    
    while (true) {
        // First maybe find the chord
        if ( !matched ) {
            for (; i < ChordCount; ++i) {
                int16_t chordCmp = compareToChord(&Chords[i], layer, keyCount, keys);
                if (chordCmp < 0) continue;
                if (chordCmp == 0) {
                    // We found the longest completed chord so far
                    *out_matchedChord = &Chords[i];
                    //Macros_SetStatusNum(i);
                    //Macros_SetStatusString("Matched!!!\n", NULL);
                    matched = true;
                    ++i;
                }
                break;
            }
        }
        
        // We now may have a match, but we can't return it yet.
        // We need to check if we have a potential for a longer chord
        // Because of the sort order of the chord list, all chords after this slot
        // with the same layer are same length or longer than the desired chord.
        
        // Now iterate until we get to the longer chords in the layer because
        // we want to see if there are potential matches in them
        if ( !noPartial && !partialMatched ) {
            while (i < ChordCount && Chords[i].layer == layer && Chords[i].keyCount == keyCount) {++i;}

            // i is now at the first longer chord in the layer, or the first key in another layer, on at list's end
            // Now we look for potential matches with more keys
            while (i < ChordCount && Chords[i].layer == layer) {
                if (isPartialOfChord(&Chords[i++], keys, keyCount)) {
                    // If we have a partial chord
                    partialMatched = true;
                }
            }
        }

        if (layer == LayerId_None) {
            break;
        }
        layer = LayerId_None;
    }

    return (matched ? ChordSearch_Exact : 0) | (partialMatched ? ChordSearch_Partial : 0);
}

void Chords_ResetChords() {
    ChordCount = 0;
}

bool setKeyHoldingChord(chord_def_t *chord, uint8_t keyId, key_pressed_state_t pressed) {
    for (uint8_t i = 0; i < chord->keyCount; ++i) {
        if (chord->keys[i] == keyId) {
            if (pressed) chord->pressedStates |= 1 << i;
            else chord->pressedStates &= ~(1 << i);

            return true;
        }
    }
    return false;
}

chord_resolution_t Chords_Driver(key_state_t *keyState, layer_id_t layer, const chord_def_t **out_matchedChord)
{
    const uint8_t thisKeyId = Utils_KeyStateToKeyId(keyState);
    if (KeyState_ActivatedNow(keyState) && ResolutionState.unrollingKeysCount > 0) {
        --ResolutionState.unrollingKeysCount;
        *out_matchedChord = ResolutionState.unrollingChord;
        setKeyHoldingChord(ResolutionState.unrollingChord, thisKeyId, KeyPressed);
        // Since the chord is unrolling, initial action on the keystroke should already have taken effect on the first key.
        // We set the chord action, but block the initial effect.  This allows the effect of keys to linger until release.
        // Except with macros, where we specifically want the initial effect on the last key.
        if ( !(ResolutionState.unrollingKeysCount == 0 && ResolutionState.unrollingChord->action.type == KeyActionType_PlayMacro) ) {
            keyState->previous = true;
        }
        return ChordResolution_Resolved;
    }
    if (Cfg.Chords_MinimumIdleTime + KeyHistory_GetLastActivationTime() > CurrentPostponedTime) {
        return ChordResolution_Failed;
    }
    if (KeyState_ActivatedNow(keyState) && ResolutionState.initialKey == NULL) {
        ResolutionState.initialKey = keyState;
        ResolutionState.pressTime = CurrentPostponedTime;
    }
    else if (ResolutionState.initialKey != keyState) {
        return ChordResolution_Failed;
    }

    chord_keys_t pressedKeys;
    uint8_t keyCount = PostponerQuery_GetPendingKeypresses(pressedKeys + 1, MAX_CHORD_KEYS - 1, ResolutionState.pressTime + Cfg.Chords_Timeout) + 1;

    pressedKeys[0] = thisKeyId;

    // It is interesting to know if we have duplicates because we cannot have chords with duplicates,
    // and the duplicate will prevent any future events from completing a chord.
    bool hasDuplicate = PostponerQuery_ContainsKeyId(pressedKeys[0]);
    for (uint8_t i = 1; i < keyCount; ++i) {
        for (uint8_t j = 0; j < i; ++j) {
            if (pressedKeys[i] == pressedKeys[j]) {
                keyCount = i;
                hasDuplicate = true;
                break;
            }
        }
    }

    const bool pressIntervalIsOver = ResolutionState.pressTime + Cfg.Chords_Timeout <= Timer_GetCurrentTime();

    chord_def_t * matchedChord = NULL;
    chord_search_result_t searchRes = searchForChordAction(&matchedChord, layer, pressedKeys, keyCount, pressIntervalIsOver || hasDuplicate);

    if (searchRes & ChordSearch_Partial) {
        // There is an unhandled edge case here:  If two chords are defined, one being a subset of the other, the smaller one cannot trigger instantly,
        // but has to wait to see if the larger one is pressed instead.  So far so good, but if a key outside the larger chord is pressed before timeout,
        // the smaller chord won't trigger at all because there is now no match anymore, with the unchorded key.
        // This could be dealt with by creeping up on the chord size every check, possibly with a cached size checked stored in ResolutionState,
        // but it's complex for a pretty limited edge case.
        EventScheduler_Schedule(ResolutionState.pressTime + Cfg.Chords_Timeout, EventSchedulerEvent_NativeActions, "NativeActions - Chord Press Interval");
        return ChordResolution_Wait;
    }

    // No reason to wait anymore, we resolve one way or the other.    
    memset((void *)&ResolutionState, 0, sizeof(ResolutionState));
    // Fake activation of the key now.
    keyState->current = true;
    keyState->previous = false;

    if (searchRes & ChordSearch_Exact) {
        *out_matchedChord = matchedChord;
        // Check that next acitvation id does not collide with an existing one since we use the activationId to identify chords.
        // Collision can cause the following
        //  - Chord activated macro checks for release may use the wrong chord and be incorrect
        //  - If colliding chords share keyIds, one will not be registered as released until pressed and released again
        // Maybe not worth the cost?  It's highly unlikely and currently only affects macro checks for release if a macro is kept active for more than 60 chord presses.
        bool collision;
        do {
            collision = false;
            if (NextActivationId == CHORDS_INVALID_ACTIVATION_ID) ++NextActivationId;
            for (uint8_t i = 0; i < ChordCount; ++i) {
                if (Chords[i].activationId == NextActivationId) {
                    if (++NextActivationId == CHORDS_INVALID_ACTIVATION_ID) {
                        ++NextActivationId;
                    }
                    // Recheck from begining
                    collision = true;
                }
            }
        } while (collision);

        matchedChord->activationId = NextActivationId++;
        matchedChord->pressedStates = 0;
        setKeyHoldingChord(matchedChord, thisKeyId, KeyPressed);
        if (Cfg.Chords_ApplicationType == ChordApplicationType_LeadingKey) {
            PostponerExtended_ConsumePendingKeypresses(keyCount - 1);
        }
        else if (Cfg.Chords_ApplicationType == ChordApplicationType_AllKeys) {
            if (matchedChord->action.type == KeyActionType_PlayMacro) {
                // Do not activate on the first key, but rather on the last.
                // This is to prevent the following issues:
                //  - activateKeyPostponed with prepend, or consumePending modifying the roll-out
                //  - the rest of the chord waiting on the queue causing problems with ifSecondary in the macro we run
                // We are still hypothetically vulnerable to other macros using those commands, but that's highly hypothetical as they would
                // likely be used while postponeKeys is active, meaning we would not be resolving anyway.
                keyState->previous = true;
            }
            ResolutionState.unrollingKeysCount = keyCount - 1;
            ResolutionState.unrollingChord = matchedChord;
        }
        return ChordResolution_Resolved;
    }

    return ChordResolution_Failed;
}

void Chords_KeyReleased(const key_state_t *keyState)
{
    for (uint8_t i = 0; i < ChordCount; ++i) {
        if (Chords[i].pressedStates == 0) continue;
        if (setKeyHoldingChord(&Chords[i], Utils_KeyStateToKeyId(keyState), KeyReleased) ) {
            if (Chords[i].pressedStates == 0) {
                Chords[i].activationId = 0;
            }
            return;
        }
    }
}

bool Chords_IsChordActivationActive(uint8_t activationId, bool checkPostponer)
{
    if (activationId == CHORDS_INVALID_ACTIVATION_ID) {
        return false;
    }
    for (uint8_t i = 0; i < ChordCount; ++i) {
        if (Chords[i].activationId == activationId) {
            for (uint8_t j = 0; j < Chords[i].keyCount; ++j) {
                if (Chords[i].pressedStates & (1 << j)) {
                    const key_state_t * const keyState = Utils_KeyIdToKeyState(Chords[i].keys[j]);
                    // Since we catch releases, if it's active, it's still the same activation.
                    // Check keystate because releases are registered after macro has it's go, so otherwise,
                    // release would not get registered for the key which applied the macro.
                    if (KeyState_Active(keyState) && (!checkPostponer || !PostponerQuery_IsKeyReleased(keyState))) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}