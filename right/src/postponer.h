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

// Macros:

    //Both 5 and 32 are quite arbitrary. 5 suffices for two keystrokes and one more event just to be sure.
    #define POSTPONER_BUFFER_SAFETY_GAP 5
    #define POSTPONER_BUFFER_SIZE 32
    #define POSTPONER_BUFFER_MAX_FILL (POSTPONER_BUFFER_SIZE-POSTPONER_BUFFER_SAFETY_GAP)

// Typedefs:

    typedef struct {
        uint32_t time;
        key_state_t * key;
        bool active;
        uint8_t layer;
    } postponer_buffer_record_type_t;

// Variables:

    extern uint8_t ChordingDelay;
    extern key_state_t* Postponer_NextEventKey;
    extern uint8_t Postponer_LastKeyLayer;

// Functions (Core hooks):

    bool PostponerCore_IsActive(void);
    void PostponerCore_PostponeNCycles(uint8_t n);
    bool PostponerCore_RunKey(key_state_t* key, bool active);
    void PostponerCore_TrackKeyEvent(key_state_t *keyState, bool active, uint8_t layer);
    void PostponerCore_RunPostponedEvents(void);
    void PostponerCore_FinishCycle(void);

// Functions (Basic Query APIs):

    uint8_t PostponerQuery_PendingKeypressCount();
    bool PostponerQuery_IsKeyReleased(key_state_t* key);
    bool PostponerQuery_IsActiveEventually(key_state_t* key);

// Functions (Query APIs extended):
    uint16_t PostponerExtended_PendingId(uint16_t idx);
    uint32_t PostponerExtended_LastPressTime(void);
    bool PostponerExtended_IsPendingKeyReleased(uint8_t idx);
    bool PostponerQuery_ContainsKeyId(uint8_t keyid);
    void PostponerExtended_ConsumePendingKeypresses(int count, bool suppress);
    void PostponerExtended_ResetPostponer(void);

    void PostponerExtended_PrintContent();

#endif /* SRC_POSTPONER_H_ */
