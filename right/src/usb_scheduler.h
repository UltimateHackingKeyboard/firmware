#ifndef __USB_SCHEDULER_H__
#define __USB_SCHEDULER_H__

// Includes:

    #include <stdint.h>
    #include "hid/transport.h" // report_sink_t

// Macros:

    // This is how much time we leave for report construction. Most of the time is
    // (probably) consumed inside the transport layers.
    //
    // If too low, we will be missing transports. If too high, we will be introducing latency.
    #define USB_REPORT_WINDOW_LOOKAHEAD_MS 3

// Variables:

    extern uint32_t UsbReportWindowEstimate;
    extern uint32_t UsbReportWindowEstimateLast;

// Functions:

    // Transport-window estimation, fed by the transport sink dispatch / sent callbacks.
    void UsbScheduler_ReportAcceptedByTransport(report_sink_t sink);
    void UsbScheduler_ReportDelivered(report_sink_t sink);

#endif
