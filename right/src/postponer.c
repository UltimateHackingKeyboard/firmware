#include "postponer.h"
#include "key_states.h"
#include "macros/key_timing.h"
#include "usb_report_updater.h"
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "timer.h"
#include "utils.h"
#include "layer_switcher.h"
#include "keymap.h"
#include "key_action.h"
#include "debug.h"
#include "config_manager.h"

postponer_buffer_record_type_t buffer[POSTPONER_BUFFER_SIZE];
static uint8_t bufferSize = 0;
static uint8_t bufferPosition = 0;

uint8_t Postponer_LastKeyLayer = 255;
uint8_t Postponer_LastKeyMods = 0;

static uint8_t cyclesUntilActivation = 0;
static uint32_t lastPressTime;

#define POS(idx) ((bufferPosition + POSTPONER_BUFFER_SIZE + (idx)) % POSTPONER_BUFFER_SIZE)

uint32_t CurrentPostponedTime = 0;

bool Postponer_MouseBlocked = false;

static void autoShift();
static void chording();


//##############################
//### Implementation Helpers ###
//##############################

static uint8_t getPendingKeypressIdx(uint8_t n)
{
    for ( int i = 0; i < bufferSize; i++ ) {
        postponer_buffer_record_type_t* rec = &buffer[POS(i)];
        if (rec->event.type == PostponerEventType_PressKey) {
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
        return buffer[POS(idx)].event.key.keyState;
    }
}

static void consumeEvent(uint8_t count)
{
    bufferPosition = POS(count);
    bufferSize = count > bufferSize ? 0 : bufferSize - count;
}

static void applyEventAndConsume(postponer_buffer_record_type_t* rec) {
    switch (rec->event.type) {
        case PostponerEventType_PressKey:
        case PostponerEventType_ReleaseKey:
            KEY_TIMING(KeyTiming_RecordKeystroke(rec->event.key.keyState, rec->event.key.active, rec->time, CurrentTime));
            rec->event.key.keyState->current = rec->event.key.active;
            Postponer_LastKeyLayer = rec->event.key.layer;
            Postponer_LastKeyMods = rec->event.key.modifiers;
            // This gives the key two ticks (this and next) to get properly processed before execution of next queued event.
            PostponerCore_PostponeNCycles(1);
            WAKE_MACROS_ON_KEYSTATE_CHANGE();
            consumeEvent(1);
            break;
        case PostponerEventType_UnblockMouse:
            Postponer_MouseBlocked = false;
            Postponer_LastKeyLayer = 255;
            Postponer_LastKeyMods = 0;
            PostponerCore_PostponeNCycles(1);
            consumeEvent(1);
            break;
        case PostponerEventType_Delay: {
            static bool delayActive = false;
            static uint32_t delayStartedAt = 0;
            if (!delayActive) {
                delayStartedAt = CurrentTime;
                delayActive = true;
            } else {
                if (Timer_GetElapsedTime(&delayStartedAt) >= rec->event.delay.length) {
                    delayActive = false;
                    consumeEvent(1);
                }
            }
            break;
        }

    }
}


static void prependEvent(postponer_event_t event)
{
    uint8_t pos = POS(-1);

    if (bufferSize == POSTPONER_BUFFER_SIZE) {
        return;
    }

    buffer[pos].event = event;
    buffer[pos].time = CurrentPostponedTime;

    lastPressTime = true
        && event.type == PostponerEventType_PressKey
        && bufferSize == 0 ? CurrentTime : lastPressTime;
    bufferSize = bufferSize < POSTPONER_BUFFER_SIZE ? bufferSize + 1 : bufferSize;
    bufferPosition--;
}

static void appendEvent(postponer_event_t event)
{

    //if the buffer is totally filled, at least make sure the key doesn't get stuck
    if (bufferSize == POSTPONER_BUFFER_SIZE) {
        applyEventAndConsume(&buffer[bufferPosition]);
    }

    uint8_t pos = POS(bufferSize);

    buffer[pos].event = event;
    buffer[pos].time = CurrentTime;

    lastPressTime = event.type == PostponerEventType_PressKey ? CurrentTime : lastPressTime;
    bufferSize = bufferSize < POSTPONER_BUFFER_SIZE ? bufferSize + 1 : bufferSize;
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
    if(bufferSize == 0 && cyclesUntilActivation == 0) {
        // ensure correct CurrentPostponedTime when postponing starts, since current postponed time is the time of last executed action
        buffer[POS(0-1+POSTPONER_BUFFER_SIZE)].time = CurrentTime;
	}
    cyclesUntilActivation = MAX(n + 1, cyclesUntilActivation);
}

bool PostponerCore_IsActive(void)
{
    return bufferSize > 0 || cyclesUntilActivation > 0 || Cfg.ChordingDelay || Cfg.AutoShiftDelay;
}


void PostponerCore_PrependKeyEvent(key_state_t *keyState, bool active, uint8_t layer)
{
    prependEvent(
                (postponer_event_t){
                    .type = active ? PostponerEventType_PressKey : PostponerEventType_ReleaseKey,
                    .key = {
                        .keyState = keyState,
                        .active = active,
                        .layer = layer,
                        .modifiers = 0,
                    }
                }
            );
}

void PostponerCore_TrackKeyEvent(key_state_t *keyState, bool active, uint8_t layer)
{
    appendEvent(
                (postponer_event_t){
                    .type = active ? PostponerEventType_PressKey : PostponerEventType_ReleaseKey,
                    .key = {
                        .keyState = keyState,
                        .active = active,
                        .layer = layer,
                        .modifiers = 0,
                    }
                }
            );
}


void PostponerCore_TrackDelay(uint32_t length)
{
    appendEvent(
                (postponer_event_t){
                    .type = PostponerEventType_Delay,
                    .delay = {
                        .length = length,
                    }
                }
            );
}


void PostponerCore_RunPostponedEvents(void)
{
    if (Cfg.ChordingDelay) {
        chording();
    }
    if (Cfg.AutoShiftDelay) {
        autoShift();
    }
    // Process one event every two cycles. (Unless someone keeps Postponer active by touching cycles_until_activation.)
    if (bufferSize != 0 && (cyclesUntilActivation == 0 || bufferSize > POSTPONER_BUFFER_MAX_FILL)) {
        CurrentPostponedTime = buffer[bufferPosition].time;
        applyEventAndConsume(&buffer[bufferPosition]);
    }
}

void PostponerCore_FinishCycle(void)
{
    cyclesUntilActivation -= cyclesUntilActivation > 0 ? 1 : 0;
    if(bufferSize == 0 && cyclesUntilActivation == 0) {
        CurrentPostponedTime = CurrentTime;
    }
}

//#######################
//### Query Functions ###
//#######################


uint8_t PostponerQuery_PendingKeypressCount()
{
    uint8_t cnt = 0;
    for ( uint8_t i = 0; i < bufferSize; i++ ) {
        if (buffer[POS(i)].event.type == PostponerEventType_PressKey) {
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
        if (buffer[POS(i)].event.type == PostponerEventType_ReleaseKey && buffer[POS(i)].event.key.keyState == key) {
            return true;
        }
    }
    return false;
}

bool PostponerQuery_IsActiveEventually(key_state_t* key)
{
    if (key == NULL) {
        return false;
    }
    for ( int8_t i = bufferSize - 1; i >= 0; i-- ) {
        if (POSTPONER_IS_KEY_EVENT(buffer[POS(i)].event.type)) {
            return buffer[POS(i)].event.key.active;
        }
    }
    return KeyState_Active(key);
}

void PostponerQuery_InfoByKeystate(key_state_t* key, postponer_buffer_record_type_t** press, postponer_buffer_record_type_t** release)
{
    *press = NULL;
    *release = NULL;
    for ( int i = 0; i < bufferSize; i++ ) {
        postponer_buffer_record_type_t* rec = &buffer[POS(i)];
        if (POSTPONER_IS_KEY_EVENT(rec->event.type) && rec->event.key.keyState == key) {
            if (rec->event.key.active) {
                *press = rec;
            } else {
                *release = rec;
                return;
            }
        }
    }
}

void PostponerQuery_InfoByQueueIdx(uint8_t idx, postponer_buffer_record_type_t** press, postponer_buffer_record_type_t** release)
{
    *press = NULL;
    *release = NULL;
    uint8_t startIdx = getPendingKeypressIdx(idx);
    if(startIdx == 255) {
        return;
    }
    *press = &buffer[POS(startIdx)];
    for ( int i = startIdx; i < bufferSize; i++ ) {
        postponer_buffer_record_type_t* rec = &buffer[POS(i)];
        if (rec->event.type == PostponerEventType_ReleaseKey && rec->event.key.keyState == (*press)->event.key.keyState) {
            *release = rec;
            return;
        }
    }
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
        if (buffer[POS(i)].event.type == PostponerEventType_PressKey && removedKeypress == NULL) {
            shifting_by++;
            removedKeypress = buffer[POS(i)].event.key.keyState;
        } else if (buffer[POS(i)].event.type == PostponerEventType_ReleaseKey && buffer[POS(i)].event.key.keyState == removedKeypress) {
            shifting_by++;
            releaseFound = true;
        }
    }
    bufferSize -= shifting_by;
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
    postponer_buffer_record_type_t* first = &buffer[POS(0)];
    postponer_buffer_record_type_t* last = &buffer[POS(bufferSize-1)];
    Macros_SetStatusString("keyid/active, size = ", NULL);
    Macros_SetStatusNum(bufferSize);
    Macros_SetStatusString("\n", NULL);
    for (int i = 0; i < POSTPONER_BUFFER_SIZE; i++) {
        postponer_buffer_record_type_t* rec = &buffer[i];
        switch(rec->event.type) {
        case PostponerEventType_PressKey:
            Macros_SetStatusString("press ", NULL);
            Macros_SetStatusNum(Utils_KeyStateToKeyId(rec->event.key.keyState));
            break;
        case PostponerEventType_ReleaseKey:
            Macros_SetStatusString("release ", NULL);
            Macros_SetStatusNum(Utils_KeyStateToKeyId(rec->event.key.keyState));
            break;
        case PostponerEventType_UnblockMouse:
            Macros_SetStatusString("unblock mouse", NULL);
            break;
        case PostponerEventType_Delay:
            Macros_SetStatusString("delay ", NULL);
            Macros_SetStatusNum(rec->event.delay.length);
            break;
        }
        if (rec == first) {
            Macros_SetStatusString(" <first", NULL);
        }
        if (rec == last) {
            Macros_SetStatusString(" <last", NULL);
        }
        Macros_SetStatusString("\n", NULL);
    }
}

bool PostponerQuery_ContainsKeyId(uint8_t keyid)
{
    key_state_t* key = Utils_KeyIdToKeyState(keyid);

    if (key == NULL) {
        return false;
    }
    for ( uint8_t i = 0; i < bufferSize; i++ ) {
        if (POSTPONER_IS_KEY_EVENT(buffer[POS(i)].event.type) && buffer[POS(i)].event.key.keyState == key) {
            return true;
        }
    }
    return false;
}

void PostponerExtended_BlockMouse() {
    Postponer_MouseBlocked = true;
}

void PostponerExtended_UnblockMouse() {
    appendEvent((postponer_event_t){ .type = PostponerEventType_UnblockMouse });
}

void PostponerExtended_RequestUnblockMouse() {
    if (Postponer_MouseBlocked) {
        SecondaryRoles_ActivateSecondaryImmediately();
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
    if (bufferSize == 0 || CurrentTime - buffer[bufferPosition].time < Cfg.ChordingDelay ) {
        PostponerCore_PostponeNCycles(0);
    } else {
        bool activated = false;
        for ( uint8_t i = 0; i < bufferSize - 1; i++ ) {
            postponer_buffer_record_type_t* a = &buffer[POS(i)];
            postponer_buffer_record_type_t* b = &buffer[POS(i+1)];
            if (POSTPONER_IS_KEY_EVENT(a->event.type) && POSTPONER_IS_KEY_EVENT(b->event.type)) {
                bool aIsActive = a->event.type == PostponerEventType_PressKey;
                bool bIsActive = b->event.type == PostponerEventType_PressKey;
                uint8_t pa = priority(a->event.key.keyState, aIsActive);
                uint8_t pb = priority(b->event.key.keyState, bIsActive);
                key_state_t* aKeyState = a->event.key.keyState;
                key_state_t* bKeyState = b->event.key.keyState;
                // Originally, this also swapped releases to go before presses.
                // Not sure why anymore, but it caused race condition on secondary role.
                if ( aIsActive && bIsActive && pa < pb ) {
                    if (aKeyState != bKeyState && b->time - a->time < Cfg.ChordingDelay) {
                        postponer_buffer_record_type_t tmp = *a;
                        *a = *b;
                        *b = tmp;
                        activated = true;
                    }
                }
            }
        }
        if (activated) {
            PostponerCore_PostponeNCycles(0);
        }
    }
}

//##########################
//### AutoShift ###
//##########################

static bool isEligibleForAutoShift()
{
    postponer_event_t* evt = &buffer[bufferPosition].event;
    key_state_t *key = evt->key.keyState;

    if (evt->type != PostponerEventType_PressKey) {
        return false;
    }

    uint8_t effectiveLayer = evt->key.layer == 255 ? ActiveLayer : evt->key.layer;

    key_action_t* a = &CurrentKeymap[effectiveLayer][0][0] + (key - &KeyStates[0][0]);

    if (a->type != KeyActionType_Keystroke) {
        return false;
    }

    if (a->keystroke.modifiers || a->keystroke.secondaryRole || a->keystroke.keystrokeType != KeystrokeType_Basic) {
        return false;
    }

    switch (a->keystroke.scancode) {
        case HID_KEYBOARD_SC_A ... HID_KEYBOARD_SC_Z:
        case HID_KEYBOARD_SC_1_AND_EXCLAMATION ... HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS:
        case HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE ... HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK:
        case HID_KEYBOARD_SC_F1 ... HID_KEYBOARD_SC_F12:
        case HID_KEYBOARD_SC_RIGHT_ARROW ... HID_KEYBOARD_SC_UP_ARROW:
        case HID_KEYBOARD_SC_HOME:
        case HID_KEYBOARD_SC_PAGE_UP:
        case HID_KEYBOARD_SC_END:
        case HID_KEYBOARD_SC_PAGE_DOWN:
            return true;
        default:
            return false;
    }

}

static void autoShift()
{
    if (bufferSize >= 1 && isEligibleForAutoShift()) {
        postponer_buffer_record_type_t *press, *release;
        key_state_t *keyState = buffer[bufferPosition].event.key.keyState;
        PostponerQuery_InfoByKeystate(keyState, &press, &release);

        if (release == NULL) {
            if ( CurrentTime - buffer[bufferPosition].time < Cfg.AutoShiftDelay ) {
                PostponerCore_PostponeNCycles(0);
            } else {
                buffer[bufferPosition].event.key.modifiers = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
            }
        }
        else if (release != NULL && release->time - press->time >= Cfg.AutoShiftDelay) {
            buffer[bufferPosition].event.key.modifiers = HID_KEYBOARD_MODIFIER_LEFTSHIFT;
        }
    } else if (bufferSize == 0) {
        PostponerCore_PostponeNCycles(0);
    }
}
