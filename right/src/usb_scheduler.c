#include <stdint.h>
#include "usb_scheduler.h"
#include "device.h"
#include "hid/transport.h"
#include "event_scheduler.h"
#include "timer.h"
#include "host_connection.h"
#include "utils.h"
#include "debug.h"

#ifdef __ZEPHYR__
#include "bt_conn.h"
#else
#include "stubs.h"
#endif

// Approximate transport window interval (ms) used by the report-construction throttle.
#define USB_REPORT_INTERVAL_MS 1

uint32_t UsbReportWindowEstimateLast = 0;
uint32_t UsbReportWindowEstimate = 0;

static uint32_t dispatchTimeMs = 0;

static uint32_t reportIntervalForSink(report_sink_t sink)
{
    switch (sink) {
    case ReportSink_Usb:
        return USB_REPORT_INTERVAL_MS;
    case ReportSink_BleHid:
    case ReportSink_Dongle:
#if DEVICE_IS_UHK80_RIGHT
        return BtConn_GetReportIntervalMs(ActiveHostConnectionId);
#else
        return 11;
#endif
    default:
        return 1;
    }
}

void UsbScheduler_ReportAcceptedByTransport(report_sink_t sink)
{
    if (DEBUG_BLE_LATENCY_STATS) {
        if (dispatchTimeMs == 0) {
            dispatchTimeMs = Timer_GetCurrentTime();
        }
    }
    UsbReportWindowEstimate = UsbReportWindowEstimateLast + 2 * reportIntervalForSink(sink);
}

void UsbScheduler_ReportDelivered(report_sink_t sink)
{
    if (DEBUG_BLE_LATENCY_STATS) {
        if (dispatchTimeMs != 0) {
            uint32_t delta = Timer_GetCurrentTime() - dispatchTimeMs;
            HidReportBleLatencyAvgMs = (HidReportBleLatencyAvgMs * 7 + delta) / 8;
            dispatchTimeMs = 0;
        }
    }
    uint32_t reportInterval = reportIntervalForSink(sink);
    uint32_t currentTime = Timer_GetCurrentTime();
    int16_t jitterGuess = (currentTime - UsbReportWindowEstimateLast - reportInterval + 1) / 2;
    jitterGuess = MAX(0, jitterGuess);
    UsbReportWindowEstimateLast = currentTime - jitterGuess;
    UsbReportWindowEstimate = currentTime - jitterGuess + reportInterval;
    uint32_t nextIn = UsbReportWindowEstimate - USB_REPORT_WINDOW_LOOKAHEAD_MS;

    if (DEVICE_IS_MASTER) {
        EventScheduler_Schedule(nextIn, EventSchedulerEvent_Postponer, "Report sent. Recalculate report throttles");
    }
}
