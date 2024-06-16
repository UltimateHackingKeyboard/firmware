#ifndef SRC_POSTPONER_H_
#define SRC_POSTPONER_H_

/**
 * Postponer function:
 *
 * Postponer is activated by `PostponeNCycles(n)` call, which makes the Postponer
 * active for next at least n cycles. This means that for at least next n update
 * cycles, key activations will be stored into Postponer's queue rather than
 * executed by usb_report_updater.
 *
 * The `n` from `PostponeNCycles(n)` is stored into `cycles_until_activation`,
 * and is decremented every update cycle. Once `cycles_until_activation` reaches
 * zero, Postponer starts replaying enqueued events at pace one event every two
 * cycles. This allows every key to go through its entire lifecycle properly.
 *
 * Postponer becomes inactive once cycles_until_activation is zero and event queue
 * is empty.
 */

// Includes:

    #include "key_states.h"
#include <stdint.h>

// Macros:

    //Both 5 and 32 are quite arbitrary. 5 suffices for two keystrokes and one more event just to be sure.
    #define POSTPONER_BUFFER_SAFETY_GAP 5
    #define POSTPONER_BUFFER_SIZE 32
    #define POSTPONER_BUFFER_MAX_FILL (POSTPONER_BUFFER_SIZE-POSTPONER_BUFFER_SAFETY_GAP)
    #define POSTPONER_IS_KEY_EVENT(E) ((E) == PostponerEventType_PressKey || (E)==PostponerEventType_ReleaseKey)

// Typedefs:

    typedef enum {
        PostponerEventType_PressKey,
        PostponerEventType_ReleaseKey,
        PostponerEventType_UnblockMouse,
        PostponerEventType_Delay,
    } postponer_event_type_t;

    typedef struct {
        union {
           struct {
            key_state_t* keyState;
            bool active;
            uint8_t layer;
            uint8_t modifiers;
           } ATTR_PACKED key;
           struct {
            uint32_t length;
           } ATTR_PACKED delay;
        };
        postponer_event_type_t type;
    } ATTR_PACKED postponer_event_t;

    typedef struct {
        uint32_t time;
        postponer_event_t event;
    } postponer_buffer_record_type_t;

// Variables:

    extern uint8_t Postponer_LastKeyLayer;
    extern uint8_t Postponer_LastKeyMods;
    extern uint32_t CurrentPostponedTime;
    extern bool Postponer_MouseBlocked;

// Functions (Core hooks):

    bool PostponerCore_IsActive(void);
    bool PostponerCore_EventsShouldBeQueued(void);
    /* void PostponerCore_PostponeNCycles(uint8_t n); */
    bool PostponerCore_RunKey(key_state_t* key, bool active);
    void PostponerCore_PrependKeyEvent(key_state_t *keyState, bool active, uint8_t layer);
    void PostponerCore_TrackKeyEvent(key_state_t *keyState, bool active, uint8_t layer);
    void PostponerCore_TrackDelay(uint32_t length) ;
    void PostponerCore_RunPostponedEvents(void);
    void PostponerCore_FinishCycle(void);

// Functions (Basic Query APIs):

    uint8_t PostponerQuery_PendingKeypressCount();
    bool PostponerQuery_IsKeyReleased(key_state_t* key);
    bool PostponerQuery_IsActiveEventually(key_state_t* key);
    void PostponerQuery_InfoByKeystate(key_state_t* key, postponer_buffer_record_type_t** press, postponer_buffer_record_type_t** release);
    void PostponerQuery_InfoByQueueIdx(uint8_t idx, postponer_buffer_record_type_t** press, postponer_buffer_record_type_t** release);
    bool PostponerQuery_ContainsKeyId(uint8_t keyid);

// Functions (Query APIs extended):
    uint16_t PostponerExtended_PendingId(uint16_t idx);
    uint32_t PostponerExtended_LastPressTime(void);
    bool PostponerExtended_IsPendingKeyReleased(uint8_t idx);
    void PostponerExtended_ConsumePendingKeypresses(int count, bool suppress);
    void PostponerExtended_ResetPostponer(void);

    void PostponerExtended_PrintContent();

    void PostponerExtended_BlockMouse();
    void PostponerExtended_UnblockMouse();
    void PostponerExtended_RequestUnblockMouse();

#endif /* SRC_POSTPONER_H_ */
