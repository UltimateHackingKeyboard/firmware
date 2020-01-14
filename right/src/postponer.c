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

void PostponerCore_PostponeNCycles(uint8_t n) {
    cycles_until_activation = MAX(n + 1, cycles_until_activation);
}

bool PostponerCore_IsActive(void) {
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
    PostponerCore_PostponeNCycles(POSTPONER_MIN_CYCLES_PER_ACTIVATION);
}

void PostponerCore_RunPostponedEvents(void)
{
    if (buffer_size != 0 && (cycles_until_activation == 0 || buffer_size > POSTPONER_BUFFER_MAX_FILL)) {
        buffer[buffer_position].key->current = buffer[buffer_position].active;
        consumeEvent(1);
        PostponerCore_PostponeNCycles(POSTPONER_MIN_CYCLES_PER_ACTIVATION);
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
    for ( int i = 0; i < buffer_size; i++ ) {
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
    for ( int i = 0; i < buffer_size; i++ ) {
        if (buffer[POS(i)].key == key && !buffer[POS(i)].active) {
            return true;
        }
    }
    return false;
}
