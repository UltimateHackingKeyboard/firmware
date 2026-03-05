#include "config_manager.h"
#include "key_history.h"
#include "postponer.h"

typedef enum {
    DoubletapState_Blocked,
    DoubletapState_First,
    DoubletapState_Multitap,
    DoubletapState_Doubletap,
} doubletap_state_t;

typedef struct {
    const key_state_t *keyState;
    uint8_t keyActivationId;
    uint32_t timestamp;
    doubletap_state_t doubletapState;
} previous_key_event_type_t;

static previous_key_event_type_t lastPress;

void KeyHistory_RecordPress(const key_state_t *keyState)
{
    const bool isMultitap = 
        keyState == lastPress.keyState
        && lastPress.doubletapState != DoubletapState_Blocked
        && CurrentPostponedTime < lastPress.timestamp + Cfg.DoubletapTimeout;
    const bool isDoubletap = isMultitap &&
        (lastPress.doubletapState == DoubletapState_First || lastPress.doubletapState == DoubletapState_Multitap);

    lastPress = (previous_key_event_type_t){
        .keyState = keyState,
        .keyActivationId = keyState->activationId,
        .timestamp = CurrentPostponedTime,
        .doubletapState = isDoubletap ? DoubletapState_Doubletap :
            isMultitap ? DoubletapState_Multitap :
            DoubletapState_First,
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

bool KeyHistory_WasLastMultitap()
{
    return lastPress.doubletapState >= DoubletapState_Multitap;
}