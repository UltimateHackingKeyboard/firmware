#ifndef __HID_TRANSPORT_H__
#define __HID_TRANSPORT_H__

#include <string.h>
#include "hid/keyboard_report.h"
#include "hid/mouse_report.h"
#include "hid/controls_report.h"

// Return type for functions that return errno-style error codes (0 = success, negative = error).
typedef int errno_t;

static inline const char* ErrToStr(errno_t err) {
    return strerror(-err);
}

typedef enum  {
    HID_TRANSPORT_USB,
    HID_TRANSPORT_BLE,
} hid_transport_t;

typedef enum
{
    ROLLOVER_N_KEY = 0,
    ROLLOVER_6_KEY = 1,
} rollover_t;


extern float HidReportBleLatencyAvgMs;

void Hid_TransportStateChanged(hid_transport_t transport, bool enabled);

// report sending
errno_t Hid_SendKeyboardReport(const hid_keyboard_report_t* report);
errno_t Hid_SendMouseReport(const hid_mouse_report_t* report);
errno_t Hid_SendControlsReport(const hid_controls_report_t* report);

void Hid_KeyboardReportSentCallback(hid_transport_t transport);
void Hid_MouseReportSentCallback(hid_transport_t transport);
void Hid_ControlsReportSentCallback(hid_transport_t transport);

// Called from NUS server 'sent' callback on UHK80 right half to feed
// dongle-bound reports into the same latency EMA as BLE HID. Assumes any
// right-half NUS send corresponds to a USB report being relayed to the dongle.
void HidTransport_NoteNusReportSent(void);

void Hid_MouseScrollResolutionsChanged(
    hid_transport_t transport, float verticalMultiplier, float horizontalMultiplier);

// num lock, caps lock, scroll lock state handling
void Hid_UpdateKeyboardLedsState(void);
void Hid_KeyboardLedsStateChanged(hid_transport_t transport);

rollover_t HID_GetKeyboardRollover(void);
void HID_SetKeyboardRollover(rollover_t mode);
bool HID_GetGamepadActive(void);
void HID_SetGamepadActive(bool active);

// USB management
void USB_SetSerialNumber(uint32_t serialNumber);
void USB_Enable(void);
bool USB_RemoteWakeup(void);
void USB_Reconfigure(void);
bool USB_IsMsHost(void);



// HOGP (BLE HID) management
void HOGP_Enable(void);
void HOGP_Disable(void);
int HOGP_HealthCheck(void);

#endif // __HID_TRANSPORT_H__
