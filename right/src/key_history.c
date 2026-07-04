#include "chords.h"
#include "config_manager.h"
#include "key_history.h"
#include "postponer.h"

typedef enum {
    HistoryEventType_Key,
    HistoryEventType_Chord,
} history_event_type_t;

typedef struct {
    const chord_def_t *chord;
    const key_state_t *chord_keys[MAX_CHORD_KEYS];
} ATTR_PACKED previous_chord_event_type_t;

typedef struct {
    history_event_type_t eventType;
    union {
        const key_state_t *key;
        previous_chord_event_type_t chord;
    } event;
    uint32_t timestamp;
    bool multiTapBroken : 1;
    uint8_t multiTapCount : 7;
} ATTR_PACKED previous_event_type_t;


previous_event_type_t lastPress;

void KeyHistory_RecordPress(const key_state_t *keyState)
{
    const bool isMultitap = 
        lastPress.eventType == HistoryEventType_Key 
        && keyState == lastPress.event.key
        && !lastPress.multiTapBroken
        && CurrentPostponedTime < lastPress.timestamp + Cfg.DoubletapTimeout;

    lastPress = (previous_event_type_t){
        .eventType = HistoryEventType_Key,
        .event = { .key = keyState },
        .timestamp = CurrentPostponedTime,
        .multiTapBroken = false,
        .multiTapCount = 1 + (isMultitap ? lastPress.multiTapCount : 0),
    };
}

void KeyHistory_RecordChordPress(const key_state_t *keyState, const chord_def_t *chord)
{
    const bool isSameChord = 
        lastPress.eventType == HistoryEventType_Chord
        && lastPress.event.chord.chord == chord;

    if (Cfg.Chords_ApplicationType == ChordApplicationType_AllKeys) {
        // There is a chance that the key press is one from the same chord activation
        // This will be the case if we can see it's the same chord as last, but we haven't seen that key
        // for that chord for that event.
        // If so, register the key as pressed as part of this chord
        bool isSameActivation = isSameChord;
        for (uint8_t i = 0; isSameActivation && i < MAX_CHORD_KEYS; ++i) {
            if (lastPress.event.chord.chord_keys[i] == keyState) {
                isSameActivation = false;
                break;
            }
            if (lastPress.event.chord.chord_keys[i] == NULL) {
                break;
            }
        }

        if (isSameActivation) {
            uint8_t i = 0;
            while (lastPress.event.chord.chord_keys[i] != NULL) ++i;
            lastPress.event.chord.chord_keys[i] = keyState;
            return;
        }
    }

    const bool isMultitap = 
        isSameChord
        && !lastPress.multiTapBroken
        && CurrentPostponedTime < lastPress.timestamp + Cfg.DoubletapTimeout;
    
    lastPress = (previous_event_type_t) {
        .eventType = HistoryEventType_Chord,
        .event.chord = {
            .chord_keys = { keyState, NULL, NULL, NULL, NULL },
            .chord = chord,
        },
        .timestamp = CurrentPostponedTime,
        .multiTapBroken = false,
        .multiTapCount = 1 + (isMultitap ? lastPress.multiTapCount : 0),
    };
}

void KeyHistory_RecordRelease(const key_state_t *keyState)
{
    if (lastPress.eventType == HistoryEventType_Key) {
        if (keyState != lastPress.event.key) {
            lastPress.multiTapBroken = true;
        }
    }
    else if (lastPress.eventType == HistoryEventType_Chord) {
        bool breaksMulti = true;
        for (uint8_t i = 0; i < MAX_CHORD_KEYS; ++i) {
            if (lastPress.event.chord.chord_keys[i] == NULL) break;
            if (lastPress.event.chord.chord_keys[i] == keyState ) {
                    breaksMulti = false;
                    break;
            }
        }
        lastPress.multiTapBroken |= breaksMulti;
    }
}

bool KeyHistory_WasLastDoubletap()
{
    return lastPress.multiTapCount % 2 == 0;
}

bool KeyHistory_WasLastMultitap()
{
    return lastPress.multiTapCount > 1;
}