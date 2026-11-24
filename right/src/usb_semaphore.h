#ifndef __USB_SEMAPHORE_H__
#define __USB_SEMAPHORE_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "hid/transport.h" // errno_t

// Macros:

    #define USB_SEMAPHORE_TIMEOUT 32 // ms

    #define USB_GIVEUP_RESEND_DELAY_MS 1024 // ms; don't spam logs or host

    // A link that doesn't confirm a report within this time is considered stalled and gets torn down.
    #define USB_SEMAPHORE_STALL_TIMEOUT 1000 // ms

// Typedefs:

    // Bits of the legacy in-flight bitfield (see `UsbSemaphore_Get`).
    typedef enum {
        UsbReportUpdate_None = 0,
        UsbReportUpdate_Keyboard = 1,
        UsbReportUpdate_Controls = 2,
        UsbReportUpdate_Mouse = 4,
    } UsbReportUpdateFlags_t;

    // How `UsbSemaphore_Release` resolves a report.
    //
    // - `Delivered`: the baseline advances to the report and its retries are reset.
    // - `Undelivered`: the baseline stays, so the report still differs from it and goes out again.
    typedef enum {
        SemaphoreRelease_Delivered,
        SemaphoreRelease_Undelivered,
    } semaphore_release_t;

    // Per-report send/resend bookkeeping, so that the send path can treat all three reports
    // uniformly.
    //
    // - `retries`: failed attempts of the current report. Stays at the maximum after a give-up,
    //   until a confirmation or a state resend.
    // - `needsResending`: the report goes out again in the next resend pass.
    // - `inFlight`: the transport took the report and hasn't confirmed it yet.
    //   - Replaces the old shared `UsbReportUpdateSemaphore` bitfield: each report owns its own
    //     byte, so setting one is a single whole-byte store (STRB on ARMv7-M) with no
    //     read-modify-write, and can't clobber another report's flag from an ISR.
    //   - `volatile`, because the transport's sent callbacks write it.
    // - `sink`, `connectionId`: where the report in flight went.
    //   - HOGP keeps an accepted report queued until it is delivered: resending there only
    //     duplicates the report and grows the queue. Such a report is waited for instead, and its
    //     connection is torn down after `USB_SEMAPHORE_STALL_TIMEOUT`.
    // - `sentTime`: when the report was handed to the transport.
    // - `switchActiveReport`: the report's double-buffer switch; advances the baseline.
    typedef struct {
        uint8_t retries;
        bool needsResending;
        volatile bool inFlight;
        report_sink_t sink;
        uint8_t connectionId;
        uint32_t sentTime;
        void (*switchActiveReport)(void);
    } report_send_state_t;

    typedef struct {
        report_send_state_t keyboard;
        report_send_state_t controls;
        report_send_state_t mouse;
    } report_send_states_t;

// Variables:

    // Exposed so the every-cycle report-construction gate can read `needsResending` as a direct
    // field access, without a function call on the idle path.
    extern report_send_states_t UsbSemaphore;

// Functions:

    // ## Taking

    // Note where a report is being sent and mark it in flight, right before handing it to the
    // transport. Before, not after: a bus reset clearing the semaphore in between would deadlock it.
    void UsbSemaphore_Take(report_send_state_t* st, report_sink_t sink, uint8_t connectionId);

    // ## Querying

    // Recompute the in-flight gate.
    //
    // - Returns false while a report is in flight within its confirmation grace window: the
    //   caller should not send.
    // - Fails any report that has timed out, which re-arms its resend.
    // - Returns true when clear to construct and send.
    bool UsbSemaphore_RecalculateIsReady(void);

    bool UsbSemaphore_AnyInFlight(void);

    // ## Releasing

    // Stop waiting for the report and unblock report processing.
    //
    // - Returns whether the report was in flight. A report is released only once: a callback can
    //   release it at any moment, so act on the release only if this returns true.
    // - Does nothing to a report that isn't in flight.
    // - `SemaphoreRelease_Delivered` is used:
    //   - USB, BLE: when the transport confirms the delivery.
    //   - Dongle link: once the link accepts the report.
    //   - For a report that is dropped on purpose (suspended USB bus).
    // - `SemaphoreRelease_Undelivered` is used when the send failed or timed out.
    bool UsbSemaphore_Release(report_send_state_t* st, semaphore_release_t how);

    // The connection went down: its unconfirmed reports will never be confirmed. Release them as
    // delivered; the current state is sent once a host is up, see
    // `UsbReportSender_RequestStateResend`.
    void UsbSemaphore_ReleaseConnection(uint8_t connectionId);

    // Release the reports sent to this sink as undelivered.
    void UsbSemaphore_ClearSink(report_sink_t sink);

    // ## Legacy

    // The in-flight flags as a `uint8_t` bitfield of `UsbReportUpdateFlags_t` - for uhk60 and for
    // usb commands.
    uint8_t UsbSemaphore_Get(void);

    void UsbSemaphore_Set(uint8_t bits);

    void UsbSemaphore_Clear(void);

#endif
