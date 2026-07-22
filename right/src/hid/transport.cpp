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
#include "jitter_test.h"
#include "key_states.h"
#include "led_display.h"
#include "logger.h"
#include "macro_events.h"
#include "test_suite/test_hooks.h"
#include "timer.h"
#include "trace.h"
#include "usb_report_updater.h"
#include "usb_scheduler.h"
#include "usb_semaphore.h"
#include "usb_state.h"
#include "utils.h"
<<<<<<< HEAD
#include "test_suite/test_hooks.h"
#include "usb_state.h"
=======
>>>>>>> multi-hogp
}
#include "command_app.hpp"
#include "controls_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#if DEVICE_IS_UHK80_RIGHT
    #include "ble_app.hpp"
#endif

#if defined(__ZEPHYR__) && (defined(CONFIG_DEBUG) == defined(NDEBUG))
    #error "Either CONFIG_DEBUG or NDEBUG must be defined"
#endif

#ifdef __ZEPHYR__
    #include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(Transport, LOG_LEVEL_INF);
#endif
// On mcux, logger.h provides the LOG_WRN / LOG_ERR / ... redirects.

static key_report_buffer keyboard_buffer;
static double_buffer<mouse_app::mouse_report_base<report_ids::IN_MOUSE>> mouse_buffer;
static double_buffer<controls_app::controls_report_base<report_ids::IN_CONTROLS>> controls_buffer;

// Exponential moving average (alpha=1/8) of the measured delay between a
// BLE HID report being handed to the stack and the corresponding sent callback
// firing. Populated when DEBUG_BLE_LATENCY_STATS is enabled (in usb_report_sender.c);
// useful for observing whether the send pipeline is saturated.
extern "C" {
float HidReportBleLatencyAvgMs = 0;
}

bool UnreliableTransportTestMode = false;

extern "C" void HidTransport_NoteNusReportSent(void)
{
    UsbScheduler_ReportDelivered(ReportSink_Dongle);
}

// TODO: maybe not only return the sink type, but also the session / conn pointer
static report_sink_t determineSink()
{
    if (TestHooks_Active) {
        return ReportSink_TestSuite;
    }

#if DEVICE_IS_UHK_DONGLE || DEVICE_IS_UHK60
    return ReportSink_Usb;
#else

    connection_type_t connectionType = Connections_Type(CurrentHostConnectionId);

    if (!Connections_IsReady(CurrentHostConnectionId)) {
        printk("Can't send report - selected connection is not ready!\n");
        Connections_HandleSwitchover(ConnectionId_Invalid, false);
        if (!Connections_IsReady(CurrentHostConnectionId)) {
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

<<<<<<< HEAD
static void wakeUsbHostIfNeeded()
{
    if (!UsbState_Awake) {
        Trace_Printf("y1.%d", CurrentPowerMode);
        USB_RemoteWakeup();
        Trace_Printc("y4");
    }
}

#ifdef __ZEPHYR__
static inline connection_id_t hidConnId(hid_transport_t transport)
=======
#if DEVICE_IS_UHK80_RIGHT
// Resolve the BLE HID session bound to the current host connection, if any.
static ble_session *currentHostBleSession()
>>>>>>> multi-hogp
{
    connection_id_t connId = ActiveHostConnectionId;
    if (connId <= ConnectionId_Invalid || connId >= ConnectionId_Count) {
        return nullptr;
    }
    uint8_t peerId = Connections[connId].peerId;
    if (peerId < PeerIdFirstHost || peerId > PeerIdLastHost) {
        return nullptr;
    }
    struct bt_conn *conn = Peers[peerId].conn;
    return conn ? ble_session::lookup_by_conn(conn) : nullptr;
}
#endif

<<<<<<< HEAD
extern "C" void Hid_TransportStateChanged(
    [[maybe_unused]] hid_transport_t transport, [[maybe_unused]] bool enabled)
{
    UsbState_SetUsbTransportUp(enabled);
#ifdef __ZEPHYR__
    Connections_SetStateAsync(hidConnId(transport), enabled ? ConnectionState_Ready : ConnectionState_Disconnected);
#endif
}
=======
// TODO: when switching sinks, or when USB only and transport comes up
// keyboard_buffer.reset_to(session->protocol(), (session->channel() == hid::channel::BLE) ? HID_ROLLOVER_N_KEY : HID_GetKeyboardRollover());
// Deferred: this hooks into the switchover path, which is being reworked in a separate PR - do it there.
>>>>>>> multi-hogp

extern "C" errno_t Hid_SendKeyboardReport(const hid_keyboard_report_t *report)
{
    // TODO: not needed when dongle is sink
    auto payload = keyboard_buffer.insert(*report);

    report_sink_t sink = determineSink();
    UsbScheduler_ReportAcceptedByTransport(sink);
    Trace_Printf("z11,%d", sink);
    errno_t err = -ECONNRESET;
    if (UnreliableTransportTestMode && Utils_Random() % 7 == 0) {
        return -EAGAIN;
    }
    switch (sink) {
    case ReportSink_Usb:
<<<<<<< HEAD
        wakeUsbHostIfNeeded();
        err = keyboard_app::usb_handle().send_report(*report);
=======
        if (auto session = keyboard_app::usb_handle().session(); session) {
            err = session->send_report(payload).to_int();
        }
>>>>>>> multi-hogp
        break;
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid: {
        hid::session *session = currentHostBleSession();

        if (!session) {
            break;
        }
        err = session->send_report(payload).to_int();
        if (err == -ENOMEM) {
            // this only happens on Android with NKRO mode when the transport MTU is too small
            printk("keyboard NKRO mode fails, falling back to 6KRO\n");

            keyboard_buffer.reset_to(hid::protocol::REPORT, rollover_t::ROLLOVER_6_KEY);
            payload = keyboard_buffer.insert(*report);
            err = session->send_report(payload).to_int();
        }
        break;
    }
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        err = Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_KeyboardReport, (const uint8_t *)report, sizeof(*report));
        if (err != 0) {
            printk("Failed to send keyboard report to dongle: %d\n", err);
        } else {
            UsbSemaphore_Release(&UsbSemaphore.keyboard);
        }
        break;
#endif
    case ReportSink_TestSuite:
        err = 0;
        TestHooks_CaptureReport(report);
        Hid_KeyboardReportSentCallback(ReportSink_Usb);
        break;
    default:
#ifdef __ZEPHYR__
        printk("Unhandled and unexpected switch state!\n");
#endif
        err = -EHOSTUNREACH;
        break;
    }
    Trace_Printf("z12,%d", err);
    if (err == 0) {
        // not needed when dongle is sink
        keyboard_buffer.swap_sides();
    }
    return err;
}

extern "C" void Hid_KeyboardReportSentCallback(report_sink_t transport)
{
    if (UnreliableTransportTestMode && Utils_Random() % 7 == 0) {
        return;
    }
    UsbSemaphore_Release(&UsbSemaphore.keyboard);
<<<<<<< HEAD
    UsbScheduler_ReportDelivered(transport == HID_TRANSPORT_USB ? ReportSink_Usb : ReportSink_BleHid);
    if (transport == HID_TRANSPORT_USB) {
        UsbState_Delivered();
    }
=======
    UsbScheduler_ReportDelivered(transport);
>>>>>>> multi-hogp
#if DEVICE_IS_UHK_DONGLE
    Dongle_SignalUsbReportSent();
#endif
}

void keyboard_report_sent_callback(hid::session &session)
{
    Hid_KeyboardReportSentCallback(
        session.channel() == hid::channel::USB ? ReportSink_Usb : ReportSink_BleHid);
}

extern "C" errno_t Hid_SendMouseReport(const hid_mouse_report_t *report)
{
    // TODO: not needed when dongle is sink
    auto payload = mouse_buffer.insert(*report);

    report_sink_t sink = determineSink();
    UsbScheduler_ReportAcceptedByTransport(sink);
    Trace_Printf("z21,%d", sink);
    errno_t err = -ECONNRESET;
    switch (sink) {
    case ReportSink_Usb:
<<<<<<< HEAD
        wakeUsbHostIfNeeded();
        err = mouse_app::usb_handle().send_report(*report);
=======
        if (auto session = mouse_app::usb_handle().session(); session) {
            err = session->send_report(payload).to_int();
        }
>>>>>>> multi-hogp
        break;
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid: {
        hid::session *session = currentHostBleSession();

        if (!session) {
            break;
        }
        err = session->send_report(payload).to_int();
        break;
    }
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        err = Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_MouseReport, (const uint8_t *)report, sizeof(*report));
        if (err != 0) {
            printk("Failed to send mouse report to dongle: %d\n", err);
        } else {
            UsbSemaphore_Release(&UsbSemaphore.mouse);
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
        // not needed when dongle is sink
        mouse_buffer.swap_sides();
    }
    return err;
}

void mouse_report_sent_callback(hid::session &session)
{
    UsbSemaphore_Release(&UsbSemaphore.mouse);
<<<<<<< HEAD
    UsbScheduler_ReportDelivered( transport == HID_TRANSPORT_USB ? ReportSink_Usb : ReportSink_BleHid);
    if (transport == HID_TRANSPORT_USB) {
        UsbState_Delivered();
    }
=======
    UsbScheduler_ReportDelivered(
        session.channel() == hid::channel::USB ? ReportSink_Usb : ReportSink_BleHid);
>>>>>>> multi-hogp
#if DEVICE_IS_UHK_DONGLE
    Dongle_SignalUsbReportSent();
#endif
}

extern "C" errno_t Hid_SendControlsReport(const hid_controls_report_t *report)
{
    // TODO: not needed when dongle is sink
    auto payload = controls_buffer.insert(*report);

    report_sink_t sink = determineSink();
    UsbScheduler_ReportAcceptedByTransport(sink);
    Trace_Printf("z31,%d", sink);
    errno_t err = -ECONNRESET;
    switch (sink) {
    case ReportSink_Usb:
<<<<<<< HEAD
        wakeUsbHostIfNeeded();
        err = controls_app::usb_handle().send_report(*report);
=======
        if (auto session = controls_app::usb_handle().session(); session) {
            err = session->send_report(payload).to_int();
        }
>>>>>>> multi-hogp
        break;
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid: {
        hid::session *session = currentHostBleSession();

        if (!session) {
            break;
        }
        err = session->send_report(payload).to_int();
        break;
    }
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        err = Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_ControlsReport, (const uint8_t *)report, sizeof(*report));
        if (err != 0) {
            printk("Failed to send controls report to dongle: %d\n", err);
        } else {
            UsbSemaphore_Release(&UsbSemaphore.controls);
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
    if (err == 0) {
        // not needed when dongle is sink
        controls_buffer.swap_sides();
    }
    return err;
}

void controls_report_sent_callback(hid::session &session)
{
    UsbSemaphore_Release(&UsbSemaphore.controls);
<<<<<<< HEAD
    UsbScheduler_ReportDelivered( transport == HID_TRANSPORT_USB ? ReportSink_Usb : ReportSink_BleHid);
    if (transport == HID_TRANSPORT_USB) {
        UsbState_Delivered();
    }
=======
    UsbScheduler_ReportDelivered(
        session.channel() == hid::channel::USB ? ReportSink_Usb : ReportSink_BleHid);
>>>>>>> multi-hogp
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
    keyboard_base_session *session = nullptr;
#ifdef __ZEPHYR__
    switch (Connections_Type(CurrentHostConnectionId)) {
    case ConnectionType_UsbHidRight:
    case ConnectionType_UsbHidLeft:
        session = keyboard_app::usb_handle().session();
        break;
    #if DEVICE_IS_UHK80_RIGHT
    case ConnectionType_BtHid:
        session = currentHostBleSession();
        break;
    #endif
    case ConnectionType_NusDongle:
        StateSync_UpdateProperty(StateSyncPropertyId_KeyboardLedsState, NULL);
        break;
    default:
        printk("Unhandled connection type %d\n", Connections_Type(CurrentHostConnectionId));
        break;
    }
#else
    session = keyboard_app::usb_handle().session();
#endif
    if (session) {
        setKeyboardLedsState(session->get_leds_report());
    }
}

void keyboard_leds_changed_callback(keyboard_base_session &session)
{
#if DEVICE_IS_UHK80_RIGHT
<<<<<<< HEAD
    auto value = (transport == HID_TRANSPORT_USB) ? keyboard_app::usb_handle().get_leds()
                                                  : keyboard_app::ble_handle().get_leds();
    connection_id_t connectionId = hidConnId(transport);
    if (Connections_IsCurrentHost(connectionId)) {
        setKeyboardLedsState(value);
=======
    connection_id_t connectionId;
    if (session.channel() == hid::channel::USB) {
        connectionId = ConnectionId_UsbHidRight;
    } else {
        struct bt_conn *conn = static_cast<ble_session &>(session).get_conn();
        int8_t peerId = conn ? GetPeerIdByConn(conn) : PeerIdUnknown;
        connectionId = (peerId >= PeerIdFirstHost && peerId <= PeerIdLastHost)
            ? (connection_id_t)Peers[peerId].connectionId
            : ConnectionId_Invalid;
    }
    if (Connections_IsActiveHostConnection(connectionId)) {
        setKeyboardLedsState(session.get_leds_report());
>>>>>>> multi-hogp
    }
#else
    setKeyboardLedsState(session.get_leds_report());
#endif
}

void mouse_resolution_changed_callback(
    hid::session &session, const mouse_session::scroll_resolution_report &report)
{
#if DEVICE_IS_UHK_DONGLE
    DongleScrollMultipliers.vertical = report.vertical_scroll_multiplier();
    DongleScrollMultipliers.horizontal = report.horizontal_scroll_multiplier();
    StateSync_UpdateProperty(StateSyncPropertyId_DongleScrollMultipliers, NULL);
#endif
}

extern "C" float VerticalScrollMultiplier(void)
{
#if !DEVICE_IS_UHK60
    switch (Connections_Type(CurrentHostConnectionId)) {
    #if DEVICE_IS_UHK80_RIGHT
    case ConnectionType_BtHid:
        if (auto *session = currentHostBleSession(); session) {
            return session->resolution_report().vertical_scroll_multiplier();
        }
        return 1.f;
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
    if (auto session = mouse_app::usb_handle().session(); session) {
        return session->resolution_report().vertical_scroll_multiplier();
    }
    return 1.f;
}

extern "C" float HorizontalScrollMultiplier(void)
{
#if !DEVICE_IS_UHK60
    switch (Connections_Type(CurrentHostConnectionId)) {
    #if DEVICE_IS_UHK80_RIGHT
    case ConnectionType_BtHid:
        if (auto *session = currentHostBleSession(); session) {
            return session->resolution_report().horizontal_scroll_multiplier();
        }
        return 1.f;
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
    if (auto session = mouse_app::usb_handle().session(); session) {
        return session->resolution_report().horizontal_scroll_multiplier();
    }
    return 1.f;
}

extern "C" void USB_Reconfigure(void);

static rollover_t keyboardRollover = rollover_t::ROLLOVER_N_KEY;

extern "C" rollover_t HID_GetKeyboardRollover()
{
    return keyboardRollover;
}

extern "C" void HID_SetKeyboardRollover(rollover_t mode)
{
    keyboardRollover = mode;
    keyboard_app::usb_handle().set_rollover(mode);
    USB_Reconfigure();
}
