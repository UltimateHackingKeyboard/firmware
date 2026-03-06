#include "config_manager.h"
#include "key_history.h"
#include "postponer.h"

#define HISTORY_SIZE 2
#define LAST (position % HISTORY_SIZE)
#define POS(p) ((position - p) % HISTORY_SIZE)

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
} key_press_event_t;

static key_press_event_t history[HISTORY_SIZE];
static uint8_t position = 0;

void KeyHistory_RecordPress(const key_state_t *keyState)
{
    const key_press_event_t * const lastPress = &history[LAST];
    const bool isMultitap = 
        keyState == lastPress->keyState
        && lastPress->doubletapState != DoubletapState_Blocked
        && CurrentPostponedTime < lastPress->timestamp + Cfg.DoubletapTimeout;
    const bool isDoubletap = isMultitap &&
        (lastPress->doubletapState == DoubletapState_First || lastPress->doubletapState == DoubletapState_Multitap);

    ++position;
    history[LAST] = (key_press_event_t) {
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
    if (keyState != history[LAST].keyState) {
        history[LAST].doubletapState = DoubletapState_Blocked;
    }
}

bool KeyHistory_WasDoubletap(const key_state_t *keyState, uint8_t activationId)
{
    for (uint8_t i = 0; i < HISTORY_SIZE; ++i) {
        const key_press_event_t * const event = &history[POS(i)];
        if (event->keyState == keyState && event->keyActivationId == activationId) {
            return event->doubletapState == DoubletapState_Doubletap;
        }
    }
    return false;
}

bool KeyHistory_WasMultitap(const key_state_t *keyState, uint8_t activationId)
{
    for (uint8_t i = 0; i < HISTORY_SIZE; ++i) {
        const key_press_event_t * const event = &history[POS(i)];
        if (event->keyState == keyState && event->keyActivationId == activationId) {
            return event->doubletapState >= DoubletapState_Multitap;
        }
    }
    return false;
}