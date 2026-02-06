#include "key_history.h"
#include "config_manager.h"
#include "postponer.h"

typedef enum {
    DoubletapState_PossibleFirst,
    DoubletapState_PossibleSecond,
    DoubletapState_IneligibleForFirst,
} doubletap_state_t;

typedef struct {
    const key_state_t *keyState;
    uint32_t timestamp;
    uint32_t previousPressTime;
    doubletap_state_t doubletapState;
} previous_key_event_type_t;

static previous_key_event_type_t lastPress;

void KeyHistory_RecordPress(const key_state_t *keyState)
{
    lastPress = (previous_key_event_type_t){
        .keyState = keyState,
        .timestamp = CurrentPostponedTime,
        .previousPressTime = lastPress.timestamp,
        .doubletapState = lastPress.doubletapState == DoubletapState_PossibleFirst ? DoubletapState_PossibleSecond : DoubletapState_PossibleFirst,
    };
}

void KeyHistory_RecordRelease(const key_state_t *keyState)
{
    if (keyState != lastPress.keyState) {
        lastPress.doubletapState = DoubletapState_IneligibleForFirst;
    }
}

bool KeyHistory_WasLastDoubletap(const uint32_t maxInterval)
{
    bool wasDoubletap = lastPress.doubletapState == DoubletapState_PossibleSecond
                        && lastPress.timestamp - lastPress.previousPressTime < maxInterval;
    if (wasDoubletap) {
        lastPress.doubletapState = DoubletapState_IneligibleForFirst;
    }
    return wasDoubletap;
}