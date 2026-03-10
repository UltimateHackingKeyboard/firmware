#include "config_manager.h"
#include "key_history.h"
#include "postponer.h"

#define HISTORY_SIZE 2
#define LAST (position % HISTORY_SIZE)
#define POS(p) ((position + HISTORY_SIZE - p) % HISTORY_SIZE) // Proceeding backwards in time

static key_press_event_t history[HISTORY_SIZE];
static uint8_t position = 0;

void KeyHistory_RecordPress(const key_state_t *keyState)
{
    const key_press_event_t * const lastPress = &history[LAST];
    const bool isMultitap = 
        keyState == lastPress->keyState
        && !lastPress->multiTapBreaker
        && CurrentPostponedTime < lastPress->timestamp + Cfg.DoubletapTimeout;

    position = (position + 1) % HISTORY_SIZE;
    
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

const key_press_event_t * KeyHistory_GetPreceedingPress(const key_state_t *keyState, uint8_t activationId)
{
    for (uint8_t i = 0; i < HISTORY_SIZE - 1; ++i) {
        if(history[POS(i)].keyState == keyState && history[POS(i)].keyActivationId == activationId) {
            if (history[POS(++i)].keyState != NULL) {
                return &history[POS(i)];
            }
            return NULL;
        }
    }
    return NULL;
}

uint8_t KeyHistory_GetMultitapCount(const key_state_t *keyState, uint8_t activationId) {
    for (uint8_t i = 0; i < HISTORY_SIZE - 1; ++i) {
        if(history[POS(i)].keyState == keyState && history[POS(i)].keyActivationId == activationId) {
            return history[POS(i)].multiTapCount;
        }
    }
    return 0;
}