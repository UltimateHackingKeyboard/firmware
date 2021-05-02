#include "postponer.h"
#include "usb_report_updater.h"
#include "macros.h"
#include "timer.h"

struct postponer_buffer_record_type_t buffer[POSTPONER_BUFFER_SIZE];
uint8_t bufferSize = 0;
uint8_t bufferPosition = 0;

uint8_t cyclesUntilActivation = 0;
key_state_t* Postponer_NextEventKey;
uint32_t lastPressTime;

#define POS(idx) ((bufferPosition + (idx)) % POSTPONER_BUFFER_SIZE)

static void consumeEvent(uint8_t count)
{
    bufferPosition = POS(count);
    bufferSize = count > bufferSize ? 0 : bufferSize - count;
    Postponer_NextEventKey = bufferSize == 0 ? NULL : buffer[bufferPosition].key;
}

//######################
//### Core Functions ###
//######################

// Postpone keys for the next n cycles. If called by multiple callers, maximum of all the
// requests is taken.
//
// 0 means "(rest of) this cycle"
// 1 means "(rest of) this cycle and the next one"
// ...
//
// E.g., if you want to stop key processing for longer time, you want to call
// this with n=1 every update cycle for as long as you want. Once you stop postponing
// the events, Postponer will start replaying them at a pace one every two cycles.
//
// If you just want to perform some action of known length without being disturbed
// (e.g., activation of a key with extra usb reports takes 2 cycles), then you just
// call this once with the required number.
void PostponerCore_PostponeNCycles(uint8_t n)
{
    cyclesUntilActivation = MAX(n + 1, cyclesUntilActivation);
}

bool PostponerCore_IsActive(void)
{
    return bufferSize > 0 || cyclesUntilActivation > 0;
}


void PostponerCore_TrackKeyEvent(key_state_t *keyState, bool active)
{
    uint8_t pos = POS(bufferSize);

    //if the buffer is totally filled, at least make sure the key doesn't get stuck
    if (bufferSize == POSTPONER_BUFFER_SIZE) {
        buffer[pos].key->current = buffer[bufferPosition].active;
        consumeEvent(1);
    }

    buffer[pos] = (struct postponer_buffer_record_type_t) {
            .key = keyState,
            .active = active,
    };
    bufferSize = bufferSize < POSTPONER_BUFFER_SIZE ? bufferSize + 1 : bufferSize;
    lastPressTime = active ? CurrentTime : lastPressTime;
}

void PostponerCore_RunPostponedEvents(void)
{
    // Process one event every two cycles. (Unless someone keeps Postponer active by touching cycles_until_activation.)
    if (bufferSize != 0 && (cyclesUntilActivation == 0 || bufferSize > POSTPONER_BUFFER_MAX_FILL)) {
        buffer[bufferPosition].key->current = buffer[bufferPosition].active;
        consumeEvent(1);
        // This gives the key two ticks (this and next) to get properly processed before execution of next queued event.
        PostponerCore_PostponeNCycles(1);
    }
}

void PostponerCore_FinishCycle(void)
{
    cyclesUntilActivation -= cyclesUntilActivation > 0 ? 1 : 0;
}

//#######################
//### Query Functions ###
//#######################


uint8_t PostponerQuery_PendingKeypressCount()
{
    uint8_t cnt = 0;
    for ( uint8_t i = 0; i < bufferSize; i++ ) {
        if (buffer[POS(i)].active) {
            cnt++;
        }
    }
    return cnt;
}

bool PostponerQuery_IsKeyReleased(key_state_t* key)
{
    if (key == NULL) {
        return false;
    }
    for ( uint8_t i = 0; i < bufferSize; i++ ) {
        if (buffer[POS(i)].key == key && !buffer[POS(i)].active) {
            return true;
        }
    }
    return false;
}
