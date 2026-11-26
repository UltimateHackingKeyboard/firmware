extern "C" {
#include "transport.h"
#ifdef __ZEPHYR__
    #include "bt_conn.h"
    #include "connections.h"
    #include "link_protocol.h"
    #include "messenger.h"
    #include "nus_server.h"
    #include "state_sync.h"
    #include <zephyr/bluetooth/gatt.h>
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
#include "usb_report_sender.h"
#include "usb_report_updater.h"
#include "usb_scheduler.h"
#include "usb_semaphore.h"
#include "usb_state.h"
#include "utils.h"
#if DEVICE_IS_UHK60
#include "i2c_watchdog.h"
#endif
}
#include "command_app.hpp"
#include "controls_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#if DEVICE_IS_UHK80_RIGHT
    #include "ble_app.hpp"
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

bool UnreliableTransportTestMode = DEBUG_STRESS_TRANSPORT;

extern "C" void Hid_DumpTransportState(void)
{
#if DEVICE_IS_UHK60
    // snapshot before our own logging deepens the stack
    uint32_t stackUsed = Debug_StackUsed();
    int32_t stackHeadroom = Debug_StackHeadroom();
#endif
    c2usb_log("hid state (t=%u ms):\n", (unsigned)Timer_GetCurrentTime());
    c2usb_log("  sessions: kb=%d mouse=%d controls=%d cmd=%d\n",
        (int)(keyboard_app::usb_handle().session() != nullptr),
        (int)(mouse_app::usb_handle().session() != nullptr),
        (int)(controls_app::usb_handle().session() != nullptr),
        (int)(command_app::usb_handle().session() != nullptr));
    c2usb_log("  powerMode=%d usbUp=%d usbSuspended=%d\n",
        (int)CurrentPowerMode, (int)UsbState_TransportUp, (int)UsbState_HostIsSuspended);
    uint32_t now = Timer_GetCurrentTime();
    c2usb_log("  key life (ms ago): scan(%d)=%u queued=%u/forceQueued=%u/applied=%u action=%u delivered=%u\n",
        (int)KeyLifeTimes.scanActive, now - KeyLifeTimes.scan, now - KeyLifeTimes.queued,
        now - KeyLifeTimes.forceQueued, now - KeyLifeTimes.applied, now - KeyLifeTimes.action,
        now - KeyLifeTimes.delivered);
    c2usb_log("  inactive kb report: %s\n", Utils_GetUsbReportString(GetInactiveKeyboardReport()));
    c2usb_log("  semaphore (inFlight/retries/needsResend): kb=%d/%d/%d ctl=%d/%d/%d mouse=%d/%d/%d\n",
        (int)UsbSemaphore.keyboard.inFlight, (int)UsbSemaphore.keyboard.retries, (int)UsbSemaphore.keyboard.needsResending,
        (int)UsbSemaphore.controls.inFlight, (int)UsbSemaphore.controls.retries, (int)UsbSemaphore.controls.needsResending,
        (int)UsbSemaphore.mouse.inFlight, (int)UsbSemaphore.mouse.retries, (int)UsbSemaphore.mouse.needsResending);
    c2usb_log("  throttle: %u ms ago blocked=%d reasons=0x%x until=in %d ms postponed=0x%x\n",
        now - MainLifeTimes.throttleTime,
        (int)MainLifeTimes.throttleBlocked, MainLifeTimes.throttleBlockReasons,
        (int)(MainLifeTimes.throttleBlockedUntil - now), (unsigned)MainLifeTimes.throttlePostponedMasks);
#if DEVICE_IS_UHK60
    c2usb_log("  isr (start/end ms ago): usb=%u/%u pit=%u/%u wdog=%u/%u i2c=%u/%u btn=%u/%u\n",
        now - IsrLifeTimes.usb.start, now - IsrLifeTimes.usb.end,
        now - IsrLifeTimes.pitTimer.start, now - IsrLifeTimes.pitTimer.end,
        now - IsrLifeTimes.i2cWatchdog.start, now - IsrLifeTimes.i2cWatchdog.end,
        now - IsrLifeTimes.i2cMain.start, now - IsrLifeTimes.i2cMain.end,
        now - IsrLifeTimes.resetButton.start, now - IsrLifeTimes.resetButton.end);
    c2usb_log("  i2c watchdog: watch=%u recoveries=%u\n",
        (unsigned)I2cWatchdog_WatchCounter, (unsigned)I2cWatchdog_RecoveryCounter);
    c2usb_log("  stack: used=%u/%u headroom=%d%s\n",
        (unsigned)stackUsed, (unsigned)Debug_StackSize(), (int)stackHeadroom,
        stackHeadroom < 0 ? " (OVERFLOW)" : "");
#endif
}

extern "C" void HidTransport_NoteNusReportSent(void)
{
    UsbScheduler_ReportDelivered(ReportSink_Dongle);
}

// TODO: maybe not only return the sink type, but also the session / conn pointer
static report_sink_t determineSink()
{
    if (TestHooks_Active && !TestHooks_AwaitingRelease) {
        return ReportSink_TestSuite;
    }

#if DEVICE_IS_UHK_DONGLE || DEVICE_IS_UHK60
    return ReportSink_Usb;
#else

    connection_type_t connectionType = Connections_Type(CurrentHostConnectionId);

    if (!Connections_IsReady(CurrentHostConnectionId)) {
        Connections_HandleSwitchover(ConnectionId_Invalid, false);
        if (!Connections_IsReady(CurrentHostConnectionId)) {
            if (connectionType == ConnectionType_UsbHidRight) {
                return ReportSink_Usb;
            } else {
                return ReportSink_BlackHole;
            }
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
        LOG_WRN("Unhandled sink type %d. Is this connection really meant to be a report target?",
            connectionType);
        return ReportSink_Usb;
    }
#endif
}

static bool isInterfaceUp(hid::session *usbSession)
{
    return determineSink() != ReportSink_Usb || usbSession != nullptr;
}

extern "C" bool Hid_IsMouseInterfaceUp(void)
{
    return isInterfaceUp(mouse_app::usb_handle().session());
}

extern "C" bool Hid_IsControlsInterfaceUp(void)
{
    return isInterfaceUp(controls_app::usb_handle().session());
}

// Returns true if a remote wakeup was signaled. The caller then drops the report, so that the waking key doesn't reach the resumed host.
static bool wakeUsbHostIfNeeded()
{
    bool wakingUp = false;
    if (UsbState_HostIsSuspended) {
        Trace_Printf("y1.%d", CurrentPowerMode);
        wakingUp = USB_RemoteWakeup();
        Trace_Printc("y4");
    }
    return wakingUp;
}

// -ENETDOWN: the bus is suspended, or our wakeup is still in progress. Drop the report instead of
// retrying it into a later resume; the current state is sent once the bus is awake, see
// UsbReportSender_RequestStateResend. On the dongle, UsbReportSender_ShouldGiveUp gives up on it at once.
static errno_t dropIfBusDown(errno_t err, report_send_state_t *st)
{
    if (DEVICE_IS_MASTER && err == -ENETDOWN) {
        UsbSemaphore_Release(st, SemaphoreRelease_Delivered);
        return 0;
    }
    return err;
}

static errno_t sendUsb(hid::session *session, std::span<const uint8_t> payload, report_send_state_t *st, bool mayWakeHost)
{
    errno_t err = -ECONNRESET;
    if (session) {
        if (mayWakeHost && wakeUsbHostIfNeeded()) {
            LOG_INF("USB host is suspended; waking it, dropping report.");
            UsbSemaphore_Release(st, SemaphoreRelease_Delivered);
            err = 0;
        } else {
            err = dropIfBusDown(session->send_report(payload).to_int(), st);
        }
    }
    return err;
}

#if DEVICE_IS_UHK80_RIGHT
static struct bt_conn *currentHostBleConn()
{
    connection_id_t connId = CurrentHostConnectionId;
    if (connId <= ConnectionId_Invalid || connId >= ConnectionId_Count) {
        return nullptr;
    }
    uint8_t peerId = Connections[connId].peerId;
    if (peerId < PeerIdFirstHost || peerId > PeerIdLastHost) {
        return nullptr;
    }
    return Peers[peerId].conn;
}

static ble_session *currentHostBleSession()
{
    struct bt_conn *conn = currentHostBleConn();
    return conn ? ble_session::lookup_by_conn(conn) : nullptr;
}

// A notification carries at most MTU - 3 bytes. The payload may start with a report ID that isn't
// sent, so this errs towards "doesn't fit".
static bool fitsBleMtu(size_t payloadSize)
{
    struct bt_conn *conn = currentHostBleConn();
    return conn && payloadSize + 3 <= bt_gatt_get_mtu(conn);
}
#endif

extern "C" void Hid_UpdateKeyboardProtocol()
{
#if !DEVICE_IS_UHK80_RIGHT
    if (auto *session = keyboard_app::usb_handle().session(); session) {
        keyboard_buffer.reset_to(session->protocol(), HID_GetKeyboardRollover());
    }
#else
    switch (Connections_Type(CurrentHostConnectionId)) {
    case ConnectionType_BtHid:
        if (auto *session = currentHostBleSession(); session) {
            keyboard_buffer.reset_to(session->protocol(), rollover_t::ROLLOVER_N_KEY);
        }
        break;
    case ConnectionType_NusDongle:
        keyboard_buffer.reset_to(hid::protocol::REPORT, rollover_t::ROLLOVER_N_KEY);
        break;
    case ConnectionType_UsbHidRight:
        if (auto *session = keyboard_app::usb_handle().session(); session) {
            keyboard_buffer.reset_to(session->protocol(), HID_GetKeyboardRollover());
        }
        break;
    default:
        break;
    }
#endif
}

static void reportSentCallback(report_send_state_t *st, report_sink_t sink, int result);

// HOGP keeps an accepted report queued until the host takes it, so it isn't resent on timeout.
static void noteSendTarget(report_send_state_t *st, report_sink_t sink)
{
#if DEVICE_IS_UHK80_RIGHT
    uint8_t connectionId = CurrentHostConnectionId;
#else
    uint8_t connectionId = 0;
#endif
    UsbSemaphore_Take(st, sink, connectionId);
}

#if DEVICE_IS_UHK80_RIGHT
static errno_t sendBle(std::span<const uint8_t> payload)
{
    hid::session *session = currentHostBleSession();
    return session ? session->send_report(payload).to_int() : -ECONNRESET;
}

static errno_t sendToDongle(report_send_state_t *st, std::span<const uint8_t> rawReport)
{
    bool isKeyboard = st == &UsbSemaphore.keyboard;
    bool isMouse = st == &UsbSemaphore.mouse;
    uint8_t propertyId = isKeyboard ? SyncablePropertyId_KeyboardReport : isMouse ? SyncablePropertyId_MouseReport : SyncablePropertyId_ControlsReport;
    errno_t err = Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty, propertyId, rawReport.data(), rawReport.size());
    if (err != 0) {
        LOG_WRN("Failed to send %s report to dongle: %d", isKeyboard ? "keyboard" : isMouse ? "mouse" : "controls", err);
    } else {
        UsbSemaphore_Release(st, SemaphoreRelease_Delivered);
    }
    return err;
}
#endif

bool Hid_TestSinkStalled = false;
static bool testSinkHolds = false;
static hid_keyboard_report_t testSinkHeldReport;

static errno_t sendToTestSink(report_send_state_t *st)
{
    errno_t err = 0;
    if (st != &UsbSemaphore.keyboard || !Hid_TestSinkStalled) {
        // Mouse and controls reports are confirmed like the keyboard ones, but not captured.
        reportSentCallback(st, ReportSink_TestSuite, 0);
    } else if (!testSinkHolds) {
        testSinkHeldReport = *ActiveKeyboardReport;
        testSinkHolds = true;
    } else {
        err = -EBUSY;
    }
    return err;
}

extern "C" bool Hid_ResumeTestSink(void)
{
    Hid_TestSinkStalled = false;
    if (testSinkHolds) {
        // The held report is what the host gets, whatever the active report is by now.
        TestHooks_CaptureReport(&testSinkHeldReport);
        reportSentCallback(&UsbSemaphore.keyboard, ReportSink_TestSuite, 0);
        testSinkHolds = false;
    }
    return true;
}

// The part of Hid_Send*Report that doesn't depend on the kind of the report. payload goes to USB
// and BLE, rawReport to the dongle.
static errno_t sendToSink(report_sink_t sink, report_send_state_t *st, hid::session *usbSession,
    std::span<const uint8_t> payload, std::span<const uint8_t> rawReport, bool mayWakeHost)
{
    errno_t err = -EHOSTUNREACH;
    switch (sink) {
    case ReportSink_Usb:
        err = sendUsb(usbSession, payload, st, mayWakeHost);
        break;
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid:
        err = sendBle(payload);
        break;
    case ReportSink_Dongle:
        err = sendToDongle(st, rawReport);
        break;
#endif
    case ReportSink_BlackHole:
        break;
    case ReportSink_TestSuite:
        err = sendToTestSink(st);
        break;
    default:
        LOG_WRN("Unhandled and unexpected switch state!");
        break;
    }
    return err;
}

template <typename T>
static std::span<const uint8_t> rawBytes(const T *report)
{
    return { (const uint8_t *)report, sizeof(*report) };
}

extern "C" errno_t Hid_SendKeyboardReport(const hid_keyboard_report_t *report)
{
    // TODO: not needed when dongle is sink
    auto payload = keyboard_buffer.insert(*report);

    report_sink_t sink = determineSink();
    noteSendTarget(&UsbSemaphore.keyboard, sink);
    UsbScheduler_ReportAcceptedByTransport(sink);
    Trace_Printf("z11,%d", sink);
    if (UnreliableTransportTestMode && Utils_Random() % UNRELIABLE_TRANSPORT_DROP_RECIPROCAL == 0) {
        return -EAGAIN;
    }
    errno_t err = sendToSink(sink, &UsbSemaphore.keyboard, keyboard_app::usb_handle().session(), payload, rawBytes(report), true);

#if DEVICE_IS_UHK80_RIGHT
    // -ENOMEM also means that the ATT buffer pool is empty; only an MTU too small for NKRO
    // (seen on Android) calls for the fallback.
    if (sink == ReportSink_BleHid && err == -ENOMEM && !fitsBleMtu(payload.size())) {
        LOG_WRN("keyboard NKRO mode fails, falling back to 6KRO");

        keyboard_buffer.reset_to(hid::protocol::REPORT, rollover_t::ROLLOVER_6_KEY);
        payload = keyboard_buffer.insert(*report);
        err = sendBle(payload);
    }
#endif
    Trace_Printf("z12,%d", err);
    if (err == 0) {
        // not needed when dongle is sink
        keyboard_buffer.swap_sides();
    }
    return err;
}

static report_sink_t sinkOf(hid::session &session)
{
    return session.channel() == hid::channel::USB ? ReportSink_Usb : ReportSink_BleHid;
}

// A delivered report is released; one whose transfer failed (e.g. the endpoint was aborted) is resent.
static void reportSentCallback(report_send_state_t *st, report_sink_t sink, int result)
{
    if (UnreliableTransportTestMode && Utils_Random() % UNRELIABLE_TRANSPORT_DROP_RECIPROCAL == 0) {
        return;
    }
    // Any confirmation, even a stale one, shows that the host takes reports of this kind again.
    if (result == 0) {
        st->retries = 0;
    }
    // A late callback of a report the semaphore no longer waits for (cleared, timed out, or sent
    // over the other transport since) would release the wrong report.
    if (!st->inFlight || st->sink != sink) {
        LOG_WRN("Ignoring a stale sent_cb(%d)", result);
        // The host took a report that was given up on, so the baseline no longer matches what the
        // host has. Send the current state.
        if (result == 0) {
            EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_ResendUsbState, "late confirmation");
        }
        return;
    }
    if (result == 0) {
        // The test suite sees a report only once its delivery is confirmed, before the release
        // switches the sent report out of ActiveKeyboardReport. A held report is captured by
        // Hid_ResumeTestSink.
        if (sink == ReportSink_TestSuite && st == &UsbSemaphore.keyboard && !testSinkHolds) {
            TestHooks_CaptureReport(ActiveKeyboardReport);
        }
        UsbSemaphore_Release(st, SemaphoreRelease_Delivered);
        UsbScheduler_ReportDelivered(sink);
    } else {
        UsbReportSender_Fail(st, result, true);
    }
#if DEVICE_IS_UHK_DONGLE
    Dongle_SignalUsbReportSent();
#endif
}

void keyboard_report_sent_callback(hid::session &session, int result)
{
    reportSentCallback(&UsbSemaphore.keyboard, sinkOf(session), result);
}

extern "C" errno_t Hid_SendMouseReport(const hid_mouse_report_t *report)
{
    // TODO: not needed when dongle is sink
    auto payload = mouse_buffer.insert(*report);

    report_sink_t sink = determineSink();
    noteSendTarget(&UsbSemaphore.mouse, sink);
    UsbScheduler_ReportAcceptedByTransport(sink);
    Trace_Printf("z21,%d", sink);

    // Only a button wakes the host, not a movement.
    bool mayWakeHost = report->buttons != 0;
    errno_t err = sendToSink(sink, &UsbSemaphore.mouse, mouse_app::usb_handle().session(), payload, rawBytes(report), mayWakeHost);
    Trace_Printf("z22,%d", err);
    if (err == 0) {
        JitterTest_RecordMouseX(report->x);
        // not needed when dongle is sink
        mouse_buffer.swap_sides();
    }
    return err;
}

void mouse_report_sent_callback(hid::session &session, int result)
{
    reportSentCallback(&UsbSemaphore.mouse, sinkOf(session), result);
}

extern "C" errno_t Hid_SendControlsReport(const hid_controls_report_t *report)
{
    // TODO: not needed when dongle is sink
    auto payload = controls_buffer.insert(*report);

    report_sink_t sink = determineSink();
    noteSendTarget(&UsbSemaphore.controls, sink);
    UsbScheduler_ReportAcceptedByTransport(sink);
    Trace_Printf("z31,%d", sink);
    errno_t err = sendToSink(sink, &UsbSemaphore.controls, controls_app::usb_handle().session(), payload, rawBytes(report), true);
    Trace_Printf("z32,%d", err);
    if (err == 0) {
        // not needed when dongle is sink
        controls_buffer.swap_sides();
    }
    return err;
}

void controls_report_sent_callback(hid::session &session, int result)
{
    reportSentCallback(&UsbSemaphore.controls, sinkOf(session), result);
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
        LOG_WRN("Unhandled connection type %d", Connections_Type(CurrentHostConnectionId));
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
    if (Connections_IsCurrentHost(connectionId)) {
        setKeyboardLedsState(session.get_leds_report());
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
