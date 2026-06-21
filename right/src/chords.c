#include "chords.h"
#include "config_manager.h"
#include "event_scheduler.h"
#include "keymap.h"
#include "layer.h"
#include "macros/status_buffer.h"
#include "postponer.h"
#include "utils.h"

#define MAX_CHORDS_COUNT 64

chord_def_t Chords[MAX_CHORDS_COUNT];
uint8_t ChordCount = 0;

static inline void sortKeys(chord_keys_t keys, uint8_t keyCount)
{
    // Some kind of bubblesort, I dunno
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
        for (layer_id_t laier = LayerId_Base; laier < LayerId_Count; ++laier) {
            handleUpdateKeysOfDeletedChordOnLayer(laier, keys, keyCount);
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
Then ascending by key count - ascending, this matters
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

chord_search_result_t searchForChordAction(key_action_t *outAction, layer_id_t layer, chord_keys_t keys, uint8_t keyCount, bool noPartial)
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
                    *outAction = Chords[i].action;
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

    if (matched && partialMatched) {
        return ChordSearch_ExactAndPartial;
    }
    if (matched) {
        // We have found an exact match
        return ChordSearch_Exact;
    }
    if (partialMatched) {
        return ChordSearch_Partial;
    }
    return ChordSearch_Nothing;
}

void Chords_ResetChords() {
    ChordCount = 0;
}

typedef enum {
    ResolutionStage_Press,
    ResolutionStage_Wait,
    ResolutionStage_Release,
} ResolutionStage;

static struct {
    key_state_t *initialKey;
    uint32_t pressTime;
    uint32_t releaseTime;
    ResolutionStage stage;
} ResolutionState;

static inline void finishResolution() {
    // Fake activation of the key now.
    ResolutionState.initialKey->current = true;
    ResolutionState.initialKey->previous = false;
    // Then clear the resolution state.
    memset((void *)&ResolutionState, 0, sizeof(ResolutionState));
}


// returns length of longest matched chord
static uint8_t matchLongestAvailableChord(key_action_t *resolvedAction, chord_keys_t keys, uint8_t count, layer_id_t layer) {
    // Cut off any trailing keys which are not part of any chords.
    for (uint8_t i = 0; i < count; ++i) {
        if (!CurrentKeymap[layer][keys[i] / 64][keys[i] % 64].isPartOfChord) {
            count = i;
            break;
        }
        // Any double represented keys means that we have spam from macros.  Stop at the dual key.
        if (i > 0) {
            for (uint8_t j = 0; j < i; ++j) {
                if (keys[j] == keys[i]) {
                    count = i;
                    break;
                }      
            }
        }
    }
    if (count < 2) {
        //Macros_SetStatusString("NoLong\n", NULL);
        return 0;
    }
    // Have to test from the bottom up to keep order, to find the longest viable chord *from the beginning*
    // Because of the sorting of the keys happening in the searcher, we can't start from the full key amount, as we need to sort of maintain order
    uint8_t lastMatch = 0;
    for (uint8_t i = 2; i <= count; ++i) {
        chord_search_result_t searchRes = searchForChordAction(resolvedAction, layer, keys, i, i == count);
        if (searchRes & ChordSearch_Exact) {
            //Macros_SetStatusNum(searchRes);
            //Macros_SetStatusString("GotExact\n", NULL);
            lastMatch = i;
        }
        if (!(searchRes & ChordSearch_Partial)) {
            break; // There are no potentially longer chords we could match with more keys
        }
    }
    //Macros_SetStatusNum(lastMatch);
    //Macros_SetStatusString("Longest\n", NULL);
    return lastMatch;
}


static chord_resolution_t runPressStage(key_state_t *keyState, layer_id_t layer, key_action_t *resolvedAction)
{
    chord_keys_t pressedKeys;
    uint8_t keyCount = PostponerQuery_GetPendingKeypresses(pressedKeys + 1, MAX_CHORD_KEYS - 1, ResolutionState.pressTime + Cfg.Chords_Timeout) + 1;

    pressedKeys[0] = Utils_KeyStateToKeyId(keyState);
    bool hasDuplicate = PostponerQuery_ContainsKeyId(pressedKeys[0]);
    for (uint8_t i = 1; i < keyCount; ++i) {
        for (uint8_t j = 0; j < i; ++j) {
            if (pressedKeys[i] == pressedKeys[j]) {
                keyCount = i;
                break;
            }
        }
    }

    const bool hasReasonToWait = Cfg.Chords_TriggerOnHold || Cfg.Chords_TriggerOnRelease;
    const bool pressIntervalIsOver = ResolutionState.pressTime + Cfg.Chords_Timeout <= Timer_GetCurrentTime();
    const bool isFinalChoice = !hasReasonToWait && pressIntervalIsOver;

    chord_search_result_t searchRes = searchForChordAction(resolvedAction, layer, pressedKeys, keyCount, isFinalChoice);

    switch (searchRes) {
    case ChordSearch_Exact:
        //Macros_SetStatusNum(Timer_GetCurrentTime() - ResolutionState.pressTime);
        //Macros_SetStatusChar('\n');
        //Macros_SetStatusString("Matched\n", NULL);
        PostponerExtended_ConsumePendingKeypresses(keyCount - 1, true);
        finishResolution();
        return ChordResolution_Resolved;
    case ChordSearch_Nothing:
        //Macros_SetStatusString("Failed\n", NULL);
        finishResolution();
        return ChordResolution_Failed;
    case ChordSearch_Partial:
        if (hasDuplicate) {
            // If we had a duplicate, it will still be there in any future checks and block matches
            finishResolution();
            return ChordResolution_Failed;
        }
        // Intentional fallthrough
    case ChordSearch_ExactAndPartial:
        //Macros_SetStatusString("Partial\n", NULL);
        if (!pressIntervalIsOver) {
            EventScheduler_Schedule(ResolutionState.pressTime + Cfg.Chords_Timeout, EventSchedulerEvent_NativeActions, "NativeActions - Chord Press Interval");
        }
        else {
            ResolutionState.stage = ResolutionStage_Wait;
            EventScheduler_Schedule(ResolutionState.pressTime + Cfg.HoldTimeout, EventSchedulerEvent_NativeActions, "NativeActions - Chord Wait For Release");
        }
        return ChordResolution_Wait;
    }
    return ChordResolution_Failed;
}


static chord_resolution_t runWaitStage(key_state_t *keyState, layer_id_t layer, key_action_t *resolvedAction)
{
        //Macros_SetStatusString("InWait\n", NULL);
    if (PostponerQuery_IsKeyReleased(keyState)) {
        //Macros_SetStatusString("Released\n", NULL);
        if (!Cfg.Chords_TriggerOnRelease) {
            finishResolution();
            return ChordResolution_Failed;
        }
        postponer_buffer_record_type_t *press, *release;
        PostponerQuery_InfoByKeystate(keyState, &press, &release);
        ResolutionState.stage = ResolutionStage_Release;
        ResolutionState.releaseTime = release->time;
        EventScheduler_Schedule(release->time + Cfg.Chords_Timeout, EventSchedulerEvent_NativeActions, "NativeActions - Chord Wait release timeout");
        return ChordResolution_Wait;
    }
    if ((ResolutionState.pressTime + Cfg.HoldTimeout <= Timer_GetCurrentTime())) {
       //Macros_SetStatusString("Held!\n", NULL);
        // Reached timeout without releasing any keys.
        if (Cfg.Chords_TriggerOnHold){
            // Resolve chord from currently held keys
            chord_keys_t keys;
            keys[0] = Utils_KeyStateToKeyId(keyState);
            // Get the start of the queue which is still being held
            uint8_t count = PostponerQuery_GetPendingHeldKeys(keys + 1, MAX_CHORD_KEYS - 1) + 1;
            for (uint8_t i = 1; i < count; ++i) {
                for (uint8_t j = 0; j < i; ++j) {
                    if (keys[i] == keys[j]) {
                        count = i;
                        break;
                    }
                }
            }

            count = matchLongestAvailableChord(resolvedAction, keys, count, layer);

            finishResolution();
            if (count == 0) {
                //Macros_SetStatusString("HeldFailed\n", NULL);
                return ChordResolution_Failed;
            }
            else {
                //Macros_SetStatusNum(count);
                //Macros_SetStatusString("HeldMatched\n", NULL);
                PostponerExtended_ConsumePendingKeypresses(count - 1, true);
                return ChordResolution_Resolved;
            }
        }

        finishResolution();
        return ChordResolution_Failed;
    }
    else {
        EventScheduler_Schedule(ResolutionState.pressTime + Cfg.HoldTimeout, EventSchedulerEvent_NativeActions, "NativeActions - Chord Wait For Release");
        return ChordResolution_Wait;
    }
}

static chord_resolution_t runReleaseStage(key_state_t *keyState, layer_id_t layer, key_action_t *resolvedAction) {
    if (ResolutionState.releaseTime + Cfg.Chords_Timeout > Timer_GetCurrentTime()) {
       //Macros_SetStatusString("ReleasedWaiting\n", NULL);
        EventScheduler_Schedule(ResolutionState.releaseTime + Cfg.Chords_Timeout, EventSchedulerEvent_NativeActions, "NativeActions - Chord Wait release timeout 2");
        return ChordResolution_Wait;
    }


       //Macros_SetStatusString("ReleasedReleased\n", NULL);
    chord_keys_t keys;
    uint8_t count = PostponerQuery_GetPendingReleaseCluster(keys, MAX_CHORD_KEYS, ResolutionState.releaseTime - Cfg.Chords_Timeout, Cfg.Chords_Timeout);
   //Macros_SetStatusNum(count);
   //Macros_SetStatusString("ReleaseCount\n", NULL);

    count = matchLongestAvailableChord(resolvedAction, keys, count, layer);

    finishResolution();
    if (count == 0) {
        return ChordResolution_Failed;
    }
    else {
        PostponerExtended_ConsumePendingKeypresses(count - 1, true);
        return ChordResolution_Resolved;
    }
}

chord_resolution_t Chords_Driver(key_state_t *keyState, layer_id_t layer, key_action_t *resolvedAction)
{
    uint32_t start = Timer_GetCurrentTimeMicros();
    if (KeyState_ActivatedNow(keyState) && ResolutionState.initialKey == NULL) {
        ResolutionState.initialKey = keyState;
        ResolutionState.pressTime = CurrentPostponedTime;
        ResolutionState.stage = ResolutionStage_Press;
    }
    else if (ResolutionState.initialKey != keyState) {
        return ChordResolution_Failed;
    }

    if (ResolutionState.stage == ResolutionStage_Press) {
        return runPressStage(keyState, layer, resolvedAction);
    }
    if (ResolutionState.stage == ResolutionStage_Wait) {
        return runWaitStage(keyState, layer, resolvedAction);
    }
    if (ResolutionState.stage == ResolutionStage_Release) {
        return runReleaseStage(keyState, layer, resolvedAction);
    }

    uint32_t elapsed = Timer_GetCurrentTimeMicros() - start;
    //Macros_SetStatusString("Processing micros: ", NULL);
    //Macros_SetStatusNum(elapsed);
    //Macros_SetStatusChar('\n');

    return ChordResolution_Failed;
}