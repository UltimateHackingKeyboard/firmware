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
 * zero, Postponer starts replaying enqueued events. After replaying an event, the
 * value is reset to POSTPONER_MIN_CYCLES_PER_ACTIVATION.
 *
 * Postponer becomes inactive once cycles_until_activation is zero and event queue
 * is empty.
 */

// Includes:

    #include "key_states.h"

// Macros:

    #define POSTPONER_BUFFER_SIZE 32
    #define POSTPONER_BUFFER_MAX_FILL (POSTPONER_BUFFER_SIZE-5)

    //It takes two cycles to send a shortcut with an extra modifier report.
    #define POSTPONER_MIN_CYCLES_PER_ACTIVATION 2

// Typedefs:

    struct postponer_buffer_record_type_t {
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
