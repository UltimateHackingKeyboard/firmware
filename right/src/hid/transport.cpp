extern "C" {
#include "transport.h"
#ifdef __ZEPHYR__
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
#include "usb_report_updater.h"
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

extern "C" int Hid_SendKeyboardReport(const hid_keyboard_report_t *report)
{
    switch (determineSink()) {
    case ReportSink_Usb:
        return keyboard_app::usb_handle().send_report(*report);
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid:
        return keyboard_app::ble_handle().send_report(*report);
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        // TODO: propagate underlying error up the stack
        Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_KeyboardReport, (const uint8_t *)report, sizeof(*report));
        return 0;
#endif
    default:
#ifdef __ZEPHYR__
        printk("Unhandled and unexpected switch state!\n");
#endif
        return -EHOSTUNREACH;
    }
}

extern "C" void Hid_KeyboardReportSentCallback(hid_transport_t transport)
{
    UsbReportUpdateSemaphore &= ~UsbReportUpdate_Keyboard;
}

extern "C" int Hid_SendMouseReport(const hid_mouse_report_t *report)
{
    switch (determineSink()) {
    case ReportSink_Usb:
        return mouse_app::usb_handle().send_report(*report);
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid:
        return mouse_app::ble_handle().send_report(*report);
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        // TODO: propagate underlying error up the stack
        Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_MouseReport, (const uint8_t *)report, sizeof(*report));
        return 0;
#endif
    default:
#ifdef __ZEPHYR__
        printk("Unhandled and unexpected switch state!\n");
#endif
        return -EHOSTUNREACH;
    }
}

extern "C" void Hid_MouseReportSentCallback(hid_transport_t transport)
{
    UsbReportUpdateSemaphore &= ~UsbReportUpdate_Mouse;
}

extern "C" int Hid_SendControlsReport(const hid_controls_report_t *report)
{
    switch (determineSink()) {
    case ReportSink_Usb:
        return controls_app::usb_handle().send_report(*report);
#if DEVICE_IS_UHK80_RIGHT
    case ReportSink_BleHid:
        return controls_app::ble_handle().send_report(*report);
#endif
#if DEVICE_IS_UHK80
    case ReportSink_Dongle:
        // TODO: propagate underlying error up the stack
        Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_ControlsReport, (const uint8_t *)report, sizeof(*report));
        return 0;
#endif
    default:
#ifdef __ZEPHYR__
        printk("Unhandled and unexpected switch state!\n");
#endif
        return -EHOSTUNREACH;
    }
}

extern "C" void Hid_ControlsReportSentCallback(hid_transport_t transport)
{
    UsbReportUpdateSemaphore &= ~UsbReportUpdate_Controls;
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
    }

#ifdef __ZEPHYR__
    StateSync_UpdateProperty(StateSyncPropertyId_KeyboardLedsState, NULL);
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
