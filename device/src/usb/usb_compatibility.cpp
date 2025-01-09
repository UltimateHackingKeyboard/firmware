extern "C" {
#include "usb_compatibility.h"
#include "bt_conn.h"
#include "debug.h"
#include "event_scheduler.h"
#include "key_states.h"
#include "macro_events.h"
#include "link_protocol.h"
#include "messenger.h"
#include "nus_server.h"
#include "state_sync.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "connections.h"
}
#include "command_app.hpp"
#include "controls_app.hpp"
#include "gamepad_app.hpp"
#include "keyboard_app.hpp"
#include "logger.h"
#include "mouse_app.hpp"
#include "usb/df/class/hid.hpp"

static controls_buffer controls;

keyboard_led_state_t KeyboardLedsState;

typedef enum {
    ReportSink_Invalid,
    ReportSink_Usb,
    ReportSink_BleHid,
    ReportSink_Dongle,
} report_sink_t;

static report_sink_t determineSink() {
    if (DEVICE_IS_UHK_DONGLE) {
        return ReportSink_Usb;
    }

    connection_type_t connectionType = Connections_Type(ActiveHostConnectionId);

    if (!Connections_IsReady(ActiveHostConnectionId)) {
        printk("Can't send report - selected connection is not ready!\n");
        Connections_HandleSwitchover(ConnectionId_Invalid, false);
        if (!Connections_IsReady(ActiveHostConnectionId)) {
            printk("Giving report to c2usb anyways!\n");
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
            printk("Unhandled sink type. Is this connection really meant to be a report target?\n");
            return ReportSink_Usb;
    }
}

extern "C" void UsbCompatibility_SendKeyboardReport(const usb_basic_keyboard_report_t* report) 
{
    switch (determineSink()) {
        case ReportSink_Usb:
            keyboard_app::usb_handle().set_report_state(*reinterpret_cast<const scancode_buffer*>(report));
            break;
#if DEVICE_IS_UHK80_RIGHT
        case ReportSink_BleHid:
            keyboard_app::ble_handle().set_report_state(*reinterpret_cast<const scancode_buffer*>(report));
            printk("Giving report to c2usb ble hid!\n");
            break;
#endif
        case ReportSink_Dongle:
            Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty, SyncablePropertyId_KeyboardReport, (const uint8_t*)report, sizeof(*report));
            break;
        default:
            printk("Unhandled and unexpected switch state!\n");
    }
}

extern "C" void UsbCompatibility_SendMouseReport(const usb_mouse_report_t *report)
{
    switch (determineSink()) {
        case ReportSink_Usb:
            mouse_app::usb_handle().set_report_state(*reinterpret_cast<const mouse_buffer*>(report));
            break;
#if DEVICE_IS_UHK80_RIGHT
        case ReportSink_BleHid:
            mouse_app::ble_handle().set_report_state(*reinterpret_cast<const mouse_buffer*>(report));
            break;
#endif
        case ReportSink_Dongle:
            Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty, SyncablePropertyId_MouseReport, (const uint8_t*)report, sizeof(*report));
            break;
        default:
            printk("Unhandled and unexpected switch state!\n");
    }
}

extern "C" void UsbCompatibility_ConsumerKeyboardAddScancode(uint8_t scancode)
{
    controls.system_codes.set(static_cast<hid::page::generic_desktop>(scancode), true);
}

extern "C" void UsbCompatibility_SendConsumerReport(const usb_media_keyboard_report_t *mediaReport,
    const usb_system_keyboard_report_t *systemReport)
{
    controls = controls_buffer();
    for (uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS && mediaReport->scancodes[i] != 0; i++) {
        controls.consumer_codes.set(
            static_cast<hid::page::consumer>(mediaReport->scancodes[i]), true);
    }
    UsbSystemKeyboard_ForeachScancode(systemReport, &UsbCompatibility_ConsumerKeyboardAddScancode);

    switch (determineSink()) {
        case ReportSink_Usb:
            controls_app::usb_handle().set_report_state(controls);
            break;
#if DEVICE_IS_UHK80_RIGHT
        case ReportSink_BleHid:
            controls_app::ble_handle().set_report_state(controls);
            break;
#endif
        case ReportSink_Dongle:
            Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty, SyncablePropertyId_ControlsReport, (const uint8_t*)&controls, sizeof(controls));
            break;
        default:
            printk("Unhandled and unexpected switch state!\n");
    }
}

extern "C" void UsbCompatibility_SendConsumerReport2(const uint8_t *report)
{
    switch (determineSink()) {
        case ReportSink_Usb:
            controls_app::usb_handle().set_report_state(*(const controls_buffer *)report);
            break;
        default:
            printk("This wasn't expected. Is this a dongle?\n");
            break;
    }
}

extern "C" void UsbCompatibility_SetKeyboardLedsState(connection_id_t connectionId, bool capsLock, bool numLock, bool scrollLock)
{
    // TODO: deal with connectionId
    if (KeyboardLedsState.capsLock != capsLock) {
        KeyboardLedsState.capsLock = capsLock;
        UsbBasicKeyboard_CapsLockOn = capsLock;
        EventVector_Set(EventVector_KeyboardLedState);
        MacroEvent_CapsLockStateChanged = true;
    }
    if (KeyboardLedsState.numLock != numLock) {
        KeyboardLedsState.numLock = numLock;
        UsbBasicKeyboard_NumLockOn = numLock;
        EventVector_Set(EventVector_KeyboardLedState);
        MacroEvent_NumLockStateChanged = true;
    }
    if (KeyboardLedsState.scrollLock != scrollLock) {
        KeyboardLedsState.scrollLock = scrollLock;
        UsbBasicKeyboard_ScrollLockOn = scrollLock;
        EventVector_Set(EventVector_KeyboardLedState);
        MacroEvent_ScrollLockStateChanged = true;
    }

    StateSync_UpdateProperty(StateSyncPropertyId_KeyboardLedsState, NULL);
}
