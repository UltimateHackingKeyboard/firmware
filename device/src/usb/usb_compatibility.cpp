extern "C" {
#include "usb_compatibility.h"
#include "bt_conn.h"
#include "debug.h"
#include "event_scheduler.h"
#include "key_states.h"
#include "link_protocol.h"
#include "macro_events.h"
#include "messenger.h"
#include "nus_server.h"
#include "state_sync.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
}
#include "command_app.hpp"
#include "controls_app.hpp"
#include "gamepad_app.hpp"
#include "keyboard_app.hpp"
#include "logger.h"
#include "mouse_app.hpp"
#include "usb/df/class/hid.hpp"

static scancode_buffer keys;
static mouse_buffer mouseState;
static controls_buffer controls;

keyboard_led_state_t KeyboardLedsState;

extern "C" void UsbCompatibility_SendKeyboardReport(const usb_basic_keyboard_report_t *report)
{
    keyboard_app &keyboard_app = keyboard_app::handle();

    if (keyboard_app.active()) {
        keyboard_app.set_report_state(*reinterpret_cast<const scancode_buffer *>(report));
    } else if (DEVICE_IS_UHK80_RIGHT && Bt_DeviceIsConnected(DeviceId_Uhk_Dongle)) {
        Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_KeyboardReport, (const uint8_t *)report, sizeof(*report));
    }
}

extern "C" void UsbCompatibility_SendMouseReport(const usb_mouse_report_t *report)
{
    mouse_app &mouse_app = mouse_app::handle();

    if (mouse_app.active()) {
        mouse_app.set_report_state(*reinterpret_cast<const mouse_buffer *>(report));
    } else if (DEVICE_IS_UHK80_RIGHT && Bt_DeviceIsConnected(DeviceId_Uhk_Dongle)) {
        Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_MouseReport, (const uint8_t *)report, sizeof(*report));
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

    controls_app &controls_app = controls_app::handle();

    if (controls_app.active()) {
        controls_app.set_report_state(controls);
    } else if (DEVICE_IS_UHK80_RIGHT && Bt_DeviceIsConnected(DeviceId_Uhk_Dongle)) {
        Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_SyncableProperty,
            SyncablePropertyId_ControlsReport, (const uint8_t *)(&controls), sizeof(controls));
    }
}

extern "C" void UsbCompatibility_SendConsumerReport2(const uint8_t *report)
{
    controls_app &controls_app = controls_app::handle();

    if (controls_app.active()) {
        controls_app.set_report_state(*(const controls_buffer *)report);
    }
}

extern "C" bool UsbCompatibility_UsbConnected()
{
    return keyboard_app::handle().has_transport();
}

extern "C" void UsbCompatibility_SetKeyboardLedsState(bool capsLock, bool numLock, bool scrollLock)
{
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

extern "C" float VerticalScrollMultiplier(void)
{
    return mouse_app::handle().resolution_report().vertical_scroll_multiplier();
}

extern "C" float HorizontalScrollMultiplier(void)
{
    return mouse_app::handle().resolution_report().horizontal_scroll_multiplier();
}
