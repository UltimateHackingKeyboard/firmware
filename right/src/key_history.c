#include "config_manager.h"
#include "key_history.h"
#include "postponer.h"

typedef enum {
    DoubletapState_PossibleFirst,
    DoubletapState_Doubletap,
    DoubletapState_Blocked,
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
    bool isDoublePress = 
        keyState == lastPress.keyState
        && lastPress.doubletapState == DoubletapState_PossibleFirst
        && CurrentPostponedTime < lastPress.timestamp + Cfg.DoubletapTimeout;

    lastPress = (previous_key_event_type_t){
        .keyState = keyState,
        .timestamp = CurrentPostponedTime,
        .doubletapState = isDoublePress ? DoubletapState_Doubletap : DoubletapState_PossibleFirst,
    };
}

void KeyHistory_RecordRelease(const key_state_t *keyState)
{
    if (keyState != lastPress.keyState) {
        lastPress.doubletapState = DoubletapState_Blocked;
    }
}

bool KeyHistory_WasLastDoubletap()
{
    return lastPress.doubletapState == DoubletapState_Doubletap;
}