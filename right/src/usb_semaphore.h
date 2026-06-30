#ifndef __USB_SEMAPHORE_H__
#define __USB_SEMAPHORE_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "hid/transport.h" // errno_t

// Macros:

    #define USB_SEMAPHORE_TIMEOUT 32 // ms
    #define USB_RESEND_DELAY_MS MAX(10, Cfg.KeystrokeDelay)

// Typedefs:

    // Bits of the legacy in-flight bitfield (see UsbSemaphore_Get).
    typedef enum {
        UsbReportUpdate_None = 0,
        UsbReportUpdate_Keyboard = 1,
        UsbReportUpdate_Controls = 2,
        UsbReportUpdate_Mouse = 4,
    } UsbReportUpdateFlags_t;

    // Per-report send/resend bookkeeping. Bundles the retry counter and resend flag with
    // the report's double-buffer switch, so the send path can treat all three reports uniformly.
    //
    // inFlight replaces the old shared UsbReportUpdateSemaphore bitfield: each report owns its
    // own byte, so setting/clearing one is a single whole-byte store (STRB on ARMv7-M) with no
    // read-modify-write, and therefore can't clobber another report's flag from an ISR. It is
    // volatile because it is written from the async transport sent callbacks. The legacy uint8
    // bitfield view (for the bus-reset reset and the debug get/set-variable command) is
    // reassembled on demand by UsbSemaphore_Get() & co.
    typedef struct {
        uint8_t retries;
        bool needsResending;
        volatile bool inFlight;
        void (*switchActiveReport)(void);
    } report_send_state_t;

    typedef struct {
        report_send_state_t keyboard;
        report_send_state_t controls;
        report_send_state_t mouse;
    } report_send_states_t;

// Variables:

    // Exposed so the every-cycle report-construction gate can read needsResending / write
    // inFlight as direct field accesses, without a function call on the idle path.
    extern report_send_states_t UsbSemaphore;

// Functions:

    // Release a report's in-flight latch: the report has been successfully sent (handed to /
    // accepted by the transport), so unblock report processing - advance the baseline to the
    // just-sent report and reset the resend state. Note this means "sent", not "delivered":
    // for USB/BLE the actual delivery confirmation is a separate, later event.
    void UsbSemaphore_Release(report_send_state_t* st);

    // Recompute the in-flight gate: returns false while a report is in flight within its
    // confirmation grace window (caller should not send), and re-arms resends for any report
    // that has timed out. Returns true when clear to construct/send.
    bool UsbSemaphore_RecalculateIsReady(void);
    bool UsbSemaphore_AnyInFlight(void);

    // Legacy uint8 bitfield view of the per-report in-flight flags (bits = UsbReportUpdateFlags_t).
    uint8_t UsbSemaphore_Get(void);
    void UsbSemaphore_Set(uint8_t bits);
    void UsbSemaphore_Clear(void);

#endif
