extern "C" {
#include "transport.h"
#ifdef __ZEPHYR__
    #include "bt_conn.h"
    #include "connections.h"
    #include "link_protocol.h"
    #include "messenger.h"
    #include "nus_server.h"
    #include "state_sync.h"
#endif
#include "debug.h"
#include "event_scheduler.h"
#include "key_states.h"
#include "logger.h"
#include "macro_events.h"
#include "timer.h"
#include "trace.h"
#include "usb_report_updater.h"
#include "led_display.h"
#include "jitter_test.h"
#include "usb_state.h"
#include "utils.h"
#include "test_suite/test_hooks.h"
}
#include "command_app.hpp"
#include "controls_app.hpp"
// #include "gamepad_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"

#if defined(__ZEPHYR__) && (defined(CONFIG_DEBUG) == defined(NDEBUG))
    #error "Either CONFIG_DEBUG or NDEBUG must be defined"
#endif

typedef enum {
    ReportSink_Invalid,
    ReportSink_Usb,
    ReportSink_BleHid,
    ReportSink_Dongle,
    ReportSink_TestSuite,
} report_sink_t;

// Exponential moving average (alpha=1/8) of the measured delay between a
// BLE HID report being handed to the stack and the corresponding sent callback
// firing. Populated when DEBUG_BLE_LATENCY_STATS is enabled; useful for
// observing whether the send pipeline is saturated.
extern "C" {
float HidReportBleLatencyAvgMs = 0;
}
static uint32_t dispatchTimeMs = 0;

// Approximate transport window intervals (ms) used by the report-construction
// throttle. After dispatch we estimate the next free window at "now + 2 *
// interval" (worst case: we just missed a window). The send-completion
// callback then reduces the estimate to "now + interval".
static constexpr uint32_t USB_REPORT_INTERVAL_MS = 1;

bool UnreliableTransportTestMode = false;

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

static void noteReportDispatched(report_sink_t sink)
{
    if (DEBUG_BLE_LATENCY_STATS) {
        if (dispatchTimeMs == 0) {
            dispatchTimeMs = Timer_GetCurrentTime();
        }
    }
    UsbReportWindowEstimate = UsbReportWindowEstimateLast + 2 * reportIntervalForSink(sink);
}

static void noteReportSent(report_sink_t transport)
{
    if (DEBUG_BLE_LATENCY_STATS) {
        if (dispatchTimeMs != 0) {
            uint32_t delta = Timer_GetCurrentTime() - dispatchTimeMs;
            HidReportBleLatencyAvgMs = (HidReportBleLatencyAvgMs * 7 + delta) / 8;
            dispatchTimeMs = 0;
        }
    }
    uint32_t reportInterval = reportIntervalForSink(transport);
    uint32_t currentTime = Timer_GetCurrentTime();
    int16_t jitterGuess = (currentTime - UsbReportWindowEstimateLast - reportInterval + 1) / 2;
    jitterGuess = MAX(0, jitterGuess);
    UsbReportWindowEstimateLast = currentTime - jitterGuess;
    UsbReportWindowEstimate = currentTime - jitterGuess + reportInterval;
    uint32_t nextIn = UsbReportWindowEstimate - USB_REPORT_WINDOW_LOOKAHEAD_MS;

    if (DEVICE_IS_UHK80_RIGHT) {
        EventScheduler_Schedule(nextIn, EventSchedulerEvent_Postponer, "Report sent. Recalculate report throttles");
    }
}

extern "C" void HidTransport_NoteNusReportSent(void)
{
    noteReportSent(ReportSink_Dongle);
}

static report_sink_t determineSink()
{
    if (TestHooks_Active) {
        return ReportSink_TestSuite;
    }

#if DEVICE_IS_UHK_DONGLE || DEVICE_IS_UHK60
    return ReportSink_Usb;
#else

    connection_type_t connectionType = Connections_Type(ActiveHostConnectionId);

    if (!Connections_IsReady(ActiveHostConnectionId)) {
        printk("Can't send report - selected connection is not ready!\n");
        Connections_HandleSwitchover(ConnectionId_Invalid, false);
        if (!Connections_IsReady(ActiveHostConnectionId)) {
            // printk("Giving report to c2usb anyways!\n");
            return ReportSink_Usb;
        }
    }

    switch (connectionType) {
    case ConnectionType_BtHid:
        return ReportSink_BleHid;
    case ConnectionType_UsbHidRight:
        return ReportSink_Usb;
    case ConnectionType_NusDongle:
        if (DEVICE_IS_UHK80_RIGHT) {
            return ReportSink_Dongle;
        }
    default:
        printk("Unhandled sink type %d. Is this connection really meant to be a report target?\n",
            connectionType);
        return ReportSink_Usb;
    }
#endif
}

#ifdef __ZEPHYR__
static inline connection_id_t hidConnId(hid_transport_t transport)
{
    if (transport == HID_TRANSPORT_USB) {
        if (DEVICE_IS_UHK80_LEFT) {
            return ConnectionId_UsbHidLeft;
        }
        return ConnectionId_UsbHidRight;
    }
    return ConnectionId_BtHid;
}
#endif

extern "C" void Hid_TransportStateChanged(
    [[maybe_unused]] hid_transport_t transport, [[maybe_unused]] bool enabled)
{
#ifdef __ZEPHYR__
    connection_id_t connId = hidConnId(transport);
    if (connId == ConnectionId_UsbHidRight) {
        UsbState_SetUsbTransportUp(enabled);
    } else {
        Connections_SetStateAsync( hidConnId(transport), enabled ? ConnectionState_Ready : ConnectionState_Disconnected);
    }
#else
    UsbState_SetUsbTransportUp(enabled);
#endif
}

extern "C" errno_t Hid_SendKeyboardReport(const hid_keyboard_report_t *report)
{
    report_sink_t sink = determineSink();
    noteReportDispatched(sink);
    Trace_Printf("z11,%d", sink);
    errno_t err;
    if (UnreliableTransportTestMode && Utils_Random() % 7 == 0) {
        return -EAGAIN;
    }
    switch (sink) {
    case ReportSink_Usb:
        err = keyboard_app::usb_handle().send_report(*report);
        break;
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid:
        err = keyboard_app::ble_handle().send_report(*report);
        break;
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        err = Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty, SyncablePropertyId_KeyboardReport, (const uint8_t *)report, sizeof(*report));
        if (err != 0) {
            printk("Failed to send keyboard report to dongle: %d\n", err);
        } else {
            UsbReportUpdater_ConfirmKeyboardReportSent();
        }
        break;
#endif
    case ReportSink_TestSuite:
        err = 0;
        TestHooks_CaptureReport(report);
        Hid_KeyboardReportSentCallback(HID_TRANSPORT_USB);
        break;
    default:
#ifdef __ZEPHYR__
        printk("Unhandled and unexpected switch state!\n");
#endif
        err = -EHOSTUNREACH;
        break;
    }
    Trace_Printf("z12,%d", err);
    return err;
}

extern "C" void Hid_KeyboardReportSentCallback(hid_transport_t transport)
{
    if (UnreliableTransportTestMode && Utils_Random() % 7 == 0) {
        return;
    }
    UsbReportUpdater_ConfirmKeyboardReportSent();
    noteReportSent(transport == HID_TRANSPORT_USB ? ReportSink_Usb : ReportSink_BleHid);
#if DEVICE_IS_UHK_DONGLE
    Dongle_SignalUsbReportSent();
#endif
}

extern "C" errno_t Hid_SendMouseReport(const hid_mouse_report_t *report)
{
    report_sink_t sink = determineSink();
    noteReportDispatched(sink);
    Trace_Printf("z21,%d", sink);
    errno_t err;
    switch (sink) {
    case ReportSink_Usb:
        err = mouse_app::usb_handle().send_report(*report);
        break;
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid:
        err = mouse_app::ble_handle().send_report(*report);
        break;
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        err = Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty, SyncablePropertyId_MouseReport, (const uint8_t *)report, sizeof(*report));
        if (err != 0) {
            printk("Failed to send mouse report to dongle: %d\n", err);
        } else {
            UsbReportUpdater_ConfirmMouseReportSent();
        }
        break;
#endif
    default:
#ifdef __ZEPHYR__
        printk("Unhandled and unexpected switch state!\n");
#endif
        err = -EHOSTUNREACH;
        break;
    }
    Trace_Printf("z22,%d", err);
    if (err == 0) {
        JitterTest_RecordMouseX(report->x);
    }
    return err;
}

extern "C" void Hid_MouseReportSentCallback(hid_transport_t transport)
{
    UsbReportUpdater_ConfirmMouseReportSent();
    noteReportSent( transport == HID_TRANSPORT_USB ? ReportSink_Usb : ReportSink_BleHid);
#if DEVICE_IS_UHK_DONGLE
    Dongle_SignalUsbReportSent();
#endif
}

extern "C" errno_t Hid_SendControlsReport(const hid_controls_report_t *report)
{
    report_sink_t sink = determineSink();
    noteReportDispatched(sink);
    Trace_Printf("z31,%d", sink);
    errno_t err;
    switch (sink) {
    case ReportSink_Usb:
        err = controls_app::usb_handle().send_report(*report);
        break;
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid:
        err = controls_app::ble_handle().send_report(*report);
        break;
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        err = Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty, SyncablePropertyId_ControlsReport, (const uint8_t *)report, sizeof(*report));
        if (err != 0) {
            printk("Failed to send controls report to dongle: %d\n", err);
        } else {
            UsbReportUpdater_ConfirmControlsReportSent();
        }
        break;
#endif
    default:
#ifdef __ZEPHYR__
        printk("Unhandled and unexpected switch state!\n");
#endif
        err = -EHOSTUNREACH;
        break;
    }
    Trace_Printf("z32,%d", err);
    return err;
}

extern "C" void Hid_ControlsReportSentCallback(hid_transport_t transport)
{
    UsbReportUpdater_ConfirmControlsReportSent();
    noteReportSent( transport == HID_TRANSPORT_USB ? ReportSink_Usb : ReportSink_BleHid);
#if DEVICE_IS_UHK_DONGLE
    Dongle_SignalUsbReportSent();
#endif
}

static void setKeyboardLedsState(hid::app::keyboard::output_report<0> report)
{
    bool changed = false;

    if (bool capsLock = report.leds.test(hid::page::leds::CAPS_LOCK);
        KeyboardLedsState.capsLock != capsLock) {
        KeyboardLedsState.capsLock = capsLock;
        changed = true;
        MacroEvent_CapsLockStateChanged = true;
    }
    if (bool numLock = report.leds.test(hid::page::leds::NUM_LOCK);
        KeyboardLedsState.numLock != numLock) {
        KeyboardLedsState.numLock = numLock;
        changed = true;
        MacroEvent_NumLockStateChanged = true;
    }
    if (bool scrollLock = report.leds.test(hid::page::leds::SCROLL_LOCK);
        KeyboardLedsState.scrollLock != scrollLock) {
        KeyboardLedsState.scrollLock = scrollLock;
        changed = true;
        MacroEvent_ScrollLockStateChanged = true;
    }
    if (changed) {
        EventVector_Set(EventVector_KeyboardLedState);
        EventVector_WakeMain();
    }

#ifdef __ZEPHYR__
    StateSync_UpdateProperty(StateSyncPropertyId_KeyboardLedsState, NULL);
#endif

#if DEVICE_IS_UHK60
    LedDisplay_SetIcon(LedDisplayIcon_CapsLock, KeyboardLedsState.capsLock);
#endif
}

extern "C" void Hid_UpdateKeyboardLedsState()
{
#ifdef __ZEPHYR__
    switch (Connections_Type(ActiveHostConnectionId)) {
    case ConnectionType_UsbHidRight:
    case ConnectionType_UsbHidLeft:
        setKeyboardLedsState(keyboard_app::usb_handle().get_leds());
        break;
    #if DEVICE_IS_UHK80_RIGHT
    case ConnectionType_BtHid:
        setKeyboardLedsState(keyboard_app::ble_handle().get_leds());
        break;
    #endif
    case ConnectionType_NusDongle:
        StateSync_UpdateProperty(StateSyncPropertyId_KeyboardLedsState, NULL);
        break;
    default:
        printk("Unhandled connection type %d\n", Connections_Type(ActiveHostConnectionId));
        break;
    }
#else
    setKeyboardLedsState(keyboard_app::usb_handle().get_leds());
#endif
}

extern "C" void Hid_KeyboardLedsStateChanged(hid_transport_t transport)
{
#if DEVICE_IS_UHK80_RIGHT
    auto value = (transport == HID_TRANSPORT_USB) ? keyboard_app::usb_handle().get_leds()
                                                  : keyboard_app::ble_handle().get_leds();
    connection_id_t connectionId = hidConnId(transport);
    if (Connections_IsActiveHostConnection(connectionId)) {
        setKeyboardLedsState(value);
    }
#else
    setKeyboardLedsState(keyboard_app::usb_handle().get_leds());
#endif
}

extern "C" void Hid_MouseScrollResolutionsChanged(
    hid_transport_t transport, float verticalMultiplier, float horizontalMultiplier)
{
#if DEVICE_IS_UHK_DONGLE
    DongleScrollMultipliers.vertical = verticalMultiplier;
    DongleScrollMultipliers.horizontal = horizontalMultiplier;
    StateSync_UpdateProperty(StateSyncPropertyId_DongleScrollMultipliers, NULL);
#endif
}

extern "C" float VerticalScrollMultiplier(void)
{
#if !DEVICE_IS_UHK60
    switch (Connections_Type(ActiveHostConnectionId)) {
    #if DEVICE_IS_UHK80_RIGHT
    case ConnectionType_BtHid:
        return mouse_app::ble_handle().resolution_report().vertical_scroll_multiplier();
    #endif
    case ConnectionType_NusDongle:
        return DongleScrollMultipliers.vertical;
    case ConnectionType_UsbHidRight:
    case ConnectionType_UsbHidLeft:
    default:
        break;
    }
#endif
    // workaround for https://github.com/UltimateHackingKeyboard/firmware/issues/1406
    if (USB_IsMsHost()) {
        return mouse_app::MAX_SCROLL_RESOLUTION;
    }
    return mouse_app::usb_handle().resolution_report().vertical_scroll_multiplier();
}

extern "C" float HorizontalScrollMultiplier(void)
{
#if !DEVICE_IS_UHK60
    switch (Connections_Type(ActiveHostConnectionId)) {
    #if DEVICE_IS_UHK80_RIGHT
    case ConnectionType_BtHid:
        return mouse_app::ble_handle().resolution_report().horizontal_scroll_multiplier();
    #endif
    case ConnectionType_NusDongle:
        return DongleScrollMultipliers.horizontal;
    case ConnectionType_UsbHidRight:
    case ConnectionType_UsbHidLeft:
    default:
        break;
    }
#endif
    // workaround for https://github.com/UltimateHackingKeyboard/firmware/issues/1406
    if (USB_IsMsHost()) {
        return mouse_app::MAX_SCROLL_RESOLUTION;
    }
    return mouse_app::usb_handle().resolution_report().horizontal_scroll_multiplier();
}

static bool gamepadActive = false;

extern "C" bool HID_GetGamepadActive()
{
    return gamepadActive;
}

extern "C" void USB_Reconfigure(void);

extern "C" void HID_SetGamepadActive(bool active)
{
    gamepadActive = active;
    USB_Reconfigure();
}

extern "C" rollover_t HID_GetKeyboardRollover()
{
    return keyboard_app::usb_handle().get_rollover();
}

extern "C" void HID_SetKeyboardRollover(rollover_t mode)
{
    keyboard_app::usb_handle().set_rollover(mode);
#if DEVICE_IS_UHK80_RIGHT
    keyboard_app::ble_handle().set_rollover(mode);
#endif
    USB_Reconfigure();
}
