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

    struct postponer_buffer_record_type_t {
        uint32_t time;
        key_state_t * key;
        bool active;
    };

// Variables:

    extern key_state_t* Postponer_NextEventKey;

// Functions (Core hooks):

    bool PostponerCore_IsActive(void);
    void PostponerCore_PostponeNCycles(uint8_t n);
    void PostponerCore_TrackKeyEvent(key_state_t *keyState, bool active);
    void PostponerCore_RunPostponedEvents(void);
    void PostponerCore_FinishCycle(void);

// Functions (Basic Query APIs):

    uint8_t PostponerQuery_PendingKeypressCount();
    bool PostponerQuery_IsKeyReleased(key_state_t* key);

#endif /* SRC_POSTPONER_H_ */
