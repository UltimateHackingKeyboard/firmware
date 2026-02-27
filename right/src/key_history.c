#include "config_manager.h"
#include "key_history.h"
#include "postponer.h"

typedef enum {
    DoubletapState_Blocked,
    DoubletapState_First,
    DoubletapState_LooseDoubletap,
    DoubletapState_StrictDoubletap,
} doubletap_state_t;

typedef struct {
    const key_state_t *keyState;
    uint8_t keyActivationId;
    uint32_t timestamp;
    uint32_t previousPressTime;
    doubletap_state_t doubletapState;
} previous_key_event_type_t;

static previous_key_event_type_t lastPress;

void KeyHistory_RecordPress(const key_state_t *keyState)
{
    const bool isDoubletap = 
        keyState == lastPress.keyState
        && CurrentPostponedTime < lastPress.timestamp + Cfg.DoubletapTimeout;
    const bool isStrictDoubletap = isDoubletap &&
        (lastPress.doubletapState == DoubletapState_First || lastPress.doubletapState == DoubletapState_LooseDoubletap);

    lastPress = (previous_key_event_type_t){
        .keyState = keyState,
        .keyActivationId = keyState->activationId,
        .timestamp = CurrentPostponedTime,
        .doubletapState = isDoubletap
            ? (isStrictDoubletap ? DoubletapState_StrictDoubletap : DoubletapState_LooseDoubletap)
            : DoubletapState_First,
    };
}

void KeyHistory_RecordRelease(const key_state_t *keyState)
{
    if (keyState != lastPress.keyState) {
        lastPress.doubletapState = DoubletapState_Blocked;
    }
}

bool KeyHistory_WasLastStrictDoubletap()
{
    return lastPress.doubletapState == DoubletapState_StrictDoubletap;
}

bool KeyHistory_WasLastLooseDoubletap()
{
    return lastPress.doubletapState >= DoubletapState_LooseDoubletap;
}