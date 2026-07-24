#pragma once

#include <string.h>
#include "hid/keyboard_report.h"
#include "hid/mouse_report.h"
#include "hid/controls_report.h"

// Return type for functions that return errno-style error codes (0 = success, negative = error).
typedef int errno_t;

static inline const char* ErrToStr(errno_t err) {
    return strerror(-err);
}

// Which physical sink a report is dispatched to. Determined by transport.c's
// determineSink(); shared so the report-sender module can size transport windows per sink.
typedef enum {
    ReportSink_Invalid,
    ReportSink_Usb,
    ReportSink_BleHid,
    ReportSink_Dongle,
    ReportSink_TestSuite,
} report_sink_t;

typedef enum
{
    ROLLOVER_N_KEY = 0,
    ROLLOVER_6_KEY = 1,
} rollover_t;

extern float HidReportBleLatencyAvgMs;
extern bool UnreliableTransportTestMode;

// report sending
errno_t Hid_SendKeyboardReport(const hid_keyboard_report_t* report);
errno_t Hid_SendMouseReport(const hid_mouse_report_t* report);
errno_t Hid_SendControlsReport(const hid_controls_report_t* report);

void Hid_KeyboardReportSentCallback(report_sink_t transport);

// Called from NUS server 'sent' callback on UHK80 right half to feed
// dongle-bound reports into the same latency EMA as BLE HID. Assumes any
// right-half NUS send corresponds to a USB report being relayed to the dongle.
void HidTransport_NoteNusReportSent(void);

// num lock, caps lock, scroll lock state handling
void Hid_UpdateKeyboardLedsState(void);

rollover_t HID_GetKeyboardRollover(void);
void HID_SetKeyboardRollover(rollover_t mode);

void Hid_UpdateKeyboardProtocol(void);

// USB management
void USB_SetSerialNumber(uint32_t serialNumber);
void USB_Enable(void);
bool USB_RemoteWakeup(void);
void USB_Reconfigure(void);
bool USB_IsMsHost(void);

// HOGP (BLE HID) management
// Instantiates the HOGP GATT service; must be called before bt_enable() so the
// static service entry is populated when Zephyr registers static services.
void HOGP_Register(void);
int HOGP_HealthCheck(void);
