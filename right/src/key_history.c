#include "key_history.h"
#include "postponer.h"

typedef enum {
    DoubletapState_Eligible,
    DoubletapState_Consumed,
    DoubletapState_Ineligible,
} doubletap_state_t;

typedef struct {
    const key_state_t *keyState;
    uint32_t timestamp;
    doubletap_state_t doubletapState: 2;
} previous_key_event_type_t;

static previous_key_event_type_t lastPress;

void KeyHistory_RecordPress(const key_state_t *keyState)
{
    lastPress = (previous_key_event_type_t){
        .keyState = keyState,
        .timestamp = CurrentPostponedTime,
        .doubletapState = lastPress.doubletapState == DoubletapState_Consumed 
            ? DoubletapState_Ineligible
            : DoubletapState_Eligible,
    };
}

void KeyHistory_RecordRelease(const key_state_t *keyState)
{
    if (keyState != lastPress.keyState) {
        lastPress.doubletapState = DoubletapState_Ineligible;
    }
}

bool KeyHistory_IsDoubletap(const key_state_t *keyState, const uint16_t doubletapInterval)
{
    uint32_t interval = CurrentPostponedTime - lastPress.timestamp;
    if(keyState != lastPress.keyState 
        || lastPress.doubletapState != DoubletapState_Eligible
        || interval > doubletapInterval) {
        return false;
    }
    lastPress.doubletapState = DoubletapState_Consumed;
    return true;
}