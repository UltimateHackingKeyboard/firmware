extern "C"
{
#include "usb_compatibility.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "link_protocol.h"
#include "legacy/debug.h"
#include "nus_server.h"
#include "messenger.h"
}
#include "usb/df/class/hid.hpp"
#include "command_app.hpp"
#include "controls_app.hpp"
#include "gamepad_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "logger.h"

static scancode_buffer keys;
static mouse_buffer mouseState;
static controls_buffer controls;

        /*
        gamepad.set_button(gamepad_button::X, KeyStates[CURRENT_SLOT_ID][3].current);
        // gamepad.left.X = 50;
        // gamepad.right.Y = 50;
        // gamepad.right_trigger = 50;
        gamepad_app::handle().set_report_state(gamepad);
        */

extern "C" void UsbCompatibility_SendKeyboardReport(const usb_basic_keyboard_report_t* report) 
{
    // the report layout is the same (as long as the report is in NKRO mode)
    keyboard_app::handle().set_report_state(*reinterpret_cast<const scancode_buffer*>(report));

    if (DEVICE_IS_UHK80_RIGHT) {
        Messenger_Send(DeviceId_Uhk_Dongle, SyncablePropertyId_KeyboardReport, (const uint8_t*)report, sizeof(*report));
    }
}

extern "C" void UsbCompatibility_SendMouseReport(const usb_mouse_report_t* report) 
{
    mouse_app::handle().set_report_state(*reinterpret_cast<const mouse_buffer*>(report));

    if (DEVICE_IS_UHK80_RIGHT) {
        Messenger_Send(DeviceId_Uhk_Dongle, SyncablePropertyId_MouseReport, (const uint8_t*)report, sizeof(*report));
    }
}

extern "C" void UsbCompatibility_ConsumerKeyboardAddScancode(uint8_t scancode) 
{
    controls.system_codes.set(static_cast<hid::page::generic_desktop>(scancode), true);
}

extern "C" void UsbCompatibility_SendConsumerReport(const usb_media_keyboard_report_t* mediaReport, const usb_system_keyboard_report_t* systemReport)
{
    controls = controls_buffer();
    for(uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS && mediaReport->scancodes[i] != 0; i++) {
        controls.consumer_codes.set(static_cast<hid::page::consumer>(mediaReport->scancodes[i]), true);
    }
    UsbSystemKeyboard_ForeachScancode(systemReport, &UsbCompatibility_ConsumerKeyboardAddScancode);

    controls_app::handle().set_report_state(controls);

    if (DEVICE_IS_UHK80_RIGHT) {
        Messenger_Send(DeviceId_Uhk_Dongle, SyncablePropertyId_ControlsReport, (const uint8_t*)(&controls), sizeof(controls));
    }
}

extern "C" void UsbCompatibility_SendConsumerReport2(const uint8_t* report)
{
    controls_app::handle().set_report_state(*(const controls_buffer*)report);
}
