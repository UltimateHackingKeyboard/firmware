#include "postponer.h"
#include "usb_report_updater.h"
#include "macros.h"
#include "timer.h"
#include "utils.h"
#include "layer_switcher.h"
#include "keymap.h"
#include "key_action.h"

struct postponer_buffer_record_type_t buffer[POSTPONER_BUFFER_SIZE];
uint8_t bufferSize = 0;
uint8_t bufferPosition = 0;

uint8_t cyclesUntilActivation = 0;
key_state_t* Postponer_NextEventKey;
uint32_t lastPressTime;

#define POS(idx) ((bufferPosition + (idx)) % POSTPONER_BUFFER_SIZE)

bool Chording = false;
static void chording();


//##############################
//### Implementation Helpers ###
//##############################

static uint8_t getPendingKeypressIdx(uint8_t n)
{
    for ( int i = 0; i < bufferSize; i++ ) {
        if (buffer[POS(i)].active) {
            if (n == 0) {
                return i;
            } else {
                n--;
            }
        }
    }
    return 255;
}

static key_state_t* getPendingKeypress(uint8_t n)
{
    uint8_t idx = getPendingKeypressIdx(n);
    if (idx == 255) {
        return NULL;
    } else {
        return buffer[POS(idx)].key;
    }
}

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
    return bufferSize > 0 || cyclesUntilActivation > 0 || Chording;
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
            .time = CurrentTime,
            .key = keyState,
            .active = active,
    };
    bufferSize = bufferSize < POSTPONER_BUFFER_SIZE ? bufferSize + 1 : bufferSize;
    /*TODO: FORK ONLY!!!*/ //postponeNCycles(POSTPONER_MIN_CYCLES_PER_ACTIVATION);
    /*TODO: FORK ONLY!!!*/ //Postponer_NextEventKey = buffer_size == 1 ? buffer[buffer_position].key : Postponer_NextEventKey;
    lastPressTime = active ? CurrentTime : lastPressTime;
}

/*
bool PostponerCore_RunKey(key_state_t* key, bool active)
{
    if (key == buffer[buffer_position].key) {
        if (cycles_until_activation == 0 || buffer_size > POSTPONER_BUFFER_MAX_FILL) {
            bool res = buffer[buffer_position].active;
            consumeEvent(1);
            postponeNCycles(POSTPONER_MIN_CYCLES_PER_ACTIVATION);
            return res;
        }
    }
    return active;
}*/


//TODO: remove either this or RunKey
void PostponerCore_RunPostponedEvents(void)
{
    if (Chording) {
        chording();
    }
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

//##########################
//### Extended Functions ###
//##########################

static void consumeOneKeypress()
{
    uint8_t shifting_by = 0;
    key_state_t* removedKeypress = NULL;
    bool releaseFound = false;
    for (int i = 0; i < bufferSize; i++) {
        buffer[POS(i-shifting_by)] = buffer[POS(i)];
        if (releaseFound) {
            continue;
        }
        if (buffer[POS(i)].active && removedKeypress == NULL) {
            shifting_by++;
            removedKeypress = buffer[POS(i)].key;
        } else if (!buffer[POS(i)].active && buffer[POS(i)].key == removedKeypress) {
            shifting_by++;
            releaseFound = true;
        }
    }
    bufferSize -= shifting_by;
    Postponer_NextEventKey = bufferSize == 0 ? NULL : buffer[bufferPosition].key;
}

void PostponerExtended_ResetPostponer(void)
{
    cyclesUntilActivation = 0;
    bufferSize = 0;
}

uint16_t PostponerExtended_PendingId(uint16_t idx)
{
    return Utils_KeyStateToKeyId(getPendingKeypress(idx));
}

uint32_t PostponerExtended_LastPressTime()
{
    return lastPressTime;
}

void PostponerExtended_ConsumePendingKeypresses(int count, bool suppress)
{
    for (int i = 0; i < count; i++) {
        consumeOneKeypress(suppress);
    }
}

bool PostponerExtended_IsPendingKeyReleased(uint8_t idx)
{
    return PostponerQuery_IsKeyReleased(getPendingKeypress(idx));
}

void PostponerExtended_PrintContent()
{
    struct postponer_buffer_record_type_t* first = &buffer[POS(0)];
    struct postponer_buffer_record_type_t* last = &buffer[POS(bufferSize-1)];
    Macros_SetStatusString("keyid/active, size = ", NULL);
    Macros_SetStatusNum(bufferSize);
    Macros_SetStatusString("\n", NULL);
    for (int i = 0; i < POSTPONER_BUFFER_SIZE; i++) {
        struct postponer_buffer_record_type_t* ptr = &buffer[i];
        Macros_SetStatusNum(Utils_KeyStateToKeyId(ptr->key));
        Macros_SetStatusString("/", NULL);
        Macros_SetStatusNum(ptr->active);
        if (ptr == first) {
            Macros_SetStatusString(" <first", NULL);
        }
        if (ptr == last) {
            Macros_SetStatusString(" <last", NULL);
        }
        Macros_SetStatusString("\n", NULL);
    }
}

//##########################
//### Chording ###
//##########################

static uint8_t priority(key_state_t *key, bool active)
{
    if (!active) {
        return 0;
    }
    key_action_t* a = &CurrentKeymap[ActiveLayer][0][0] + (key - &KeyStates[0][0]);
    switch (a->type) {
        case KeyActionType_Keystroke:
            if (a->keystroke.secondaryRole || a->keystroke.scancode == 0) {
                return 1;
            }
            return 0;
            break;
        case KeyActionType_Mouse:
            return 0;
        case KeyActionType_SwitchLayer:
        case KeyActionType_SwitchKeymap:
            return 2;
        case KeyActionType_PlayMacro:
            return 1;
    }
    return 0;
}

static void chording()
{
    const uint16_t limit = 50;
    if (bufferSize == 0 || CurrentTime - buffer[bufferPosition].time < limit ) {
        PostponerCore_PostponeNCycles(0);
    } else {
        bool activated = false;
        for ( uint8_t i = 0; i < bufferSize - 1; i++ ) {
            struct postponer_buffer_record_type_t* a = &buffer[POS(i)];
            struct postponer_buffer_record_type_t* b = &buffer[POS(i+1)];
            uint8_t pa = priority(a->key, a->active);
            uint8_t pb = priority(b->key, b->active);
            if ( (a->active && !b->active) || (a->active && b->active && pa < pb) ) {
                if (a->key != b->key && b->time - a->time < limit) {
                    struct postponer_buffer_record_type_t tmp = *a;
                    *a = *b;
                    *b = tmp;
                    activated = true;
                }
            }
        }
        if (activated) {
            PostponerCore_PostponeNCycles(0);
        }
    }
}
