#include "config_manager.h"
#include "key_history.h"
#include "postponer.h"

#define HISTORY_SIZE 2
#define LAST (position % HISTORY_SIZE)
#define POS(p) ((position - p) % HISTORY_SIZE)

typedef struct {
    const key_state_t *keyState;
    uint32_t timestamp;
    uint8_t multiTapCount;
    uint8_t keyActivationId : 4;
    bool multiTapBreaker : 1;
} key_press_event_t;

static key_press_event_t history[HISTORY_SIZE];
static uint8_t position = 0;

void KeyHistory_RecordPress(const key_state_t *keyState)
{
    const key_press_event_t * const lastPress = &history[LAST];
    const bool isMultitap = 
        keyState == lastPress->keyState
        && !lastPress->multiTapBreaker
        && CurrentPostponedTime < lastPress->timestamp + Cfg.DoubletapTimeout;

    ++position;
    history[LAST] = (key_press_event_t) {
        .keyState = keyState,
        .keyActivationId = keyState->activationId,
        .timestamp = CurrentPostponedTime,
        .multiTapCount = 1 + (isMultitap ? lastPress->multiTapCount : 0),
        .multiTapBreaker = false,
    };
}

void KeyHistory_RecordRelease(const key_state_t *keyState)
{
    if (keyState != history[LAST].keyState) {
        history[LAST].multiTapBreaker = true;
    }
}

uint8_t KeyHistory_GetMultitapCount(const key_state_t *keyState, uint8_t activationId) {
    for (uint8_t i = 0; i < HISTORY_SIZE - 1; ++i) {
        if(history[POS(i)].keyState == keyState && history[POS(i)].keyActivationId == activationId) {
            return history[POS(i)].multiTapCount;
        }
    }
    return 0;
}