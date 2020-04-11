#include "postponer.h"
#include "usb_report_updater.h"
#include "macros.h"
#include "timer.h"

struct postponer_buffer_record_type_t buffer[POSTPONER_BUFFER_SIZE];
uint8_t buffer_size = 0;
uint8_t buffer_position = 0;

uint8_t cycles_until_activation = 0;
key_state_t* Postponer_NextEventKey;
uint32_t last_press_time;

#define POS(idx) ((buffer_position + (idx)) % POSTPONER_BUFFER_SIZE)

static void consumeEvent(uint8_t count)
{
    buffer_position = POS(count);
    buffer_size = count > buffer_size ? 0 : buffer_size - count;
    Postponer_NextEventKey = buffer_size == 0 ? NULL : buffer[buffer_position].key;
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
    cycles_until_activation = MAX(n + 1, cycles_until_activation);
}

bool PostponerCore_IsActive(void)
{
    return buffer_size > 0 || cycles_until_activation > 0;
}


void PostponerCore_TrackKeyEvent(key_state_t *keyState, bool active)
{
    uint8_t pos = POS(buffer_size);
    buffer[pos] = (struct postponer_buffer_record_type_t) {
            .key = keyState,
            .active = active,
    };
    buffer_size = buffer_size < POSTPONER_BUFFER_SIZE ? buffer_size + 1 : buffer_size;
    last_press_time = active ? CurrentTime : last_press_time;
}

void PostponerCore_RunPostponedEvents(void)
{
    // Process one event every two cycles. (Unless someone keeps Postponer active by touching cycles_until_activation.)
    if (buffer_size != 0 && (cycles_until_activation == 0 || buffer_size > POSTPONER_BUFFER_MAX_FILL)) {
        buffer[buffer_position].key->current = buffer[buffer_position].active;
        consumeEvent(1);
        // This gives the key two ticks (this and next) to get properly processed before execution of next queued event.
        PostponerCore_PostponeNCycles(1);
    }
}

void PostponerCore_FinishCycle(void)
{
    cycles_until_activation -= cycles_until_activation > 0 ? 1 : 0;
}

//#######################
//### Query Functions ###
//#######################


uint8_t PostponerQuery_PendingKeypressCount()
{
    uint8_t cnt = 0;
    for ( uint8_t i = 0; i < buffer_size; i++ ) {
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
    for ( uint8_t i = 0; i < buffer_size; i++ ) {
        if (buffer[POS(i)].key == key && !buffer[POS(i)].active) {
            return true;
        }
    }
    return false;
}
