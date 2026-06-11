extern "C" {
#include "transport.h"
#ifdef __ZEPHYR__
    #include "connections.h"
    #include "bt_conn.h"
    #include "link_protocol.h"
    #include "messenger.h"
    #include "nus_server.h"
    #include "state_sync.h"
    #include <bluetooth/radio_notification_cb.h>
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
} report_sink_t;

// Exponential moving average (alpha=1/8) of the measured delay between a
// BLE HID report being handed to the stack and the corresponding sent callback
// firing. Populated when DEBUG_BLE_LATENCY_STATS is enabled; useful for
// observing whether the send pipeline is saturated.
extern "C" {
float HidReportBleLatencyAvgMs = 0;
}
static uint32_t dispatchTimeMs = 0;

// Report-construction timing is no longer estimated from the report-sent
// callback. Instead, on UHK80 right we build exactly one report per BLE
// connection event, triggered by the radio-notification "prepare" callback a
// fixed time before each connection-event anchor point (see the report-anchor
// section below). noteReportDispatched/noteReportSent now only maintain the
// optional BLE latency EMA.

static void noteReportDispatched(report_sink_t sink)
{
    (void)sink;
    if (DEBUG_BLE_LATENCY_STATS) {
        if (dispatchTimeMs == 0) {
            dispatchTimeMs = Timer_GetCurrentTime();
        }
    }
}

static void noteReportSent(report_sink_t transport)
{
    (void)transport;
    if (DEBUG_BLE_LATENCY_STATS) {
        if (dispatchTimeMs != 0) {
            uint32_t delta = Timer_GetCurrentTime() - dispatchTimeMs;
            HidReportBleLatencyAvgMs = (HidReportBleLatencyAvgMs * 7 + delta) / 8;
            dispatchTimeMs = 0;
        }
    }
    EventVector_WakeMain();
}

extern "C" void HidTransport_NoteNusReportSent(void)
{
    noteReportSent(ReportSink_Dongle);
}

// ---------------------------------------------------------------------------
// Report anchor (UHK80 right)
//
// The radio-notification library invokes reportAnchorPrepare() from the system
// workqueue a fixed distance (BT_RADIO..._PREPARE_DISTANCE_US) before each
// connection-event anchor point. We use it to open a one-shot "window" that
// allows the report updater to build and submit a single report for the
// upcoming event. The window is consumed by the updater (see
// blockedByReportThrottle), so at most one report is produced per event.
// ---------------------------------------------------------------------------
#if DEVICE_IS_UHK80_RIGHT
extern "C" {
// Set by the anchor prepare callback (system workqueue), consumed by the report
// updater (main loop). Single-writer each direction; a lost/duplicated update
// only costs/gains one event, so a plain volatile flag is sufficient.
volatile bool ReportAnchorWindowOpen = false;

// Set by the report updater when it blocks waiting for the next anchor window
// (it cannot rely on the prepare callback observing EventVector_SendUsbReports
// directly, because the throttle postpones that bit out of the live vector).
// Tells the prepare callback whether a pending report is waiting on the window.
volatile bool ReportAnchorWaiting = false;
}

// True when reports for the active host go out over a BLE link (and therefore
// have anchor-driven connection events). USB sinks have no anchor and are not
// gated.
extern "C" bool ReportAnchor_IsActive(void)
{
    switch (Connections_Type(ActiveHostConnectionId)) {
    case ConnectionType_BtHid:
    case ConnectionType_NusDongle:
        return true;
    default:
        return false;
    }
}

static void reportAnchorPrepare(struct bt_conn *conn)
{
    int8_t peerId = GetPeerIdByConn(conn);
    if (peerId == PeerIdUnknown) {
        return;
    }
    connection_id_t connectionId = (connection_id_t)Peers[peerId].connectionId;

    // Only the connection we actually send reports to should drive the window.
    if (!Connections_IsActiveHostConnection(connectionId)) {
        return;
    }

    // TEMP DEBUG: record this anchor fire into the jitter timeline so its cadence
    // can be seen interleaved with report builds. windowWasOpen flags a fire that
    // found the previous window still unconsumed (a missed/late build).
    if (JitterTest_Active) {
        JitterTest_RecordAnchor(ReportAnchorWindowOpen);
    }

    ReportAnchorWindowOpen = true;
    if (ReportAnchorWaiting) {
        // Hand main a *live* work bit (not just a semaphore poke): while waiting
        // for the window the throttle has postponed the report bits out of the
        // live vector, so RunUserLogic would otherwise see no work and sleep
        // again. Setting SendUsbReports live makes RunUserLogic run
        // UpdateUsbReports, and because it is live at the gate the window is
        // consumed on the first pass (no double build).
        EventVector_Set(EventVector_SendUsbReports);
        EventVector_WakeMain();
    }
}

static const struct bt_radio_notification_conn_cb reportAnchorCb = {
    .prepare = reportAnchorPrepare,
};

// Distance from the prepare callback to the connection-event anchor. Must cover
// system-workqueue scheduling + main wakeup + report build + submit-to-LL, plus
// peripheral clock drift. (Recommended default is 3000us.)
#define REPORT_ANCHOR_PREPARE_DISTANCE_US 5000

extern "C" void HidTransport_InitReportAnchor(void)
{
    int err = bt_radio_notification_conn_cb_register(
        &reportAnchorCb, REPORT_ANCHOR_PREPARE_DISTANCE_US);
    if (err) {
        printk("Failed to register report anchor notification callback: %d\n", err);
    }
}
#endif

static report_sink_t determineSink()
{
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
    Connections_SetState(
        hidConnId(transport), enabled ? ConnectionState_Ready : ConnectionState_Disconnected);
#endif
}

extern "C" errno_t Hid_SendKeyboardReport(const hid_keyboard_report_t *report)
{
    report_sink_t sink = determineSink();
    noteReportDispatched(sink);
    Trace_Printf("z11,%d", sink);
    errno_t err;
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
    Trace_Printf("z12,%d", err);
    return err;
}

extern "C" void Hid_KeyboardReportSentCallback(hid_transport_t transport)
{
    UsbReportUpdateSemaphore &= ~UsbReportUpdate_Keyboard;
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
    UsbReportUpdateSemaphore &= ~UsbReportUpdate_Mouse;
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
    UsbReportUpdateSemaphore &= ~UsbReportUpdate_Controls;
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
