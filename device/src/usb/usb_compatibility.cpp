extern "C"
{
#include "usb_compatibility.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "legacy/debug.h"
}
#include "usb/df/class/hid.hpp"
#include "command_app.hpp"
#include "controls_app.hpp"
#include "gamepad_app.hpp"
#include "keyboard_app.hpp"
#include "mouse_app.hpp"
#include "keyboard/logger.h"

static scancode_buffer keys;
static mouse_buffer mouseState;
static controls_buffer controls;

extern "C" void UsbCompatibility_KeyboardAddScancode(uint8_t scancode) 
{
    keys.set_code(static_cast<hid::page::keyboard_keypad>(scancode), true);
}

extern "C" void UsbCompatibility_SendKeyboardReport(usb_basic_keyboard_report_t* report) 
{
    keys = scancode_buffer();

    if (report->modifiers & HID_KEYBOARD_MODIFIER_RIGHTCTRL) {
        keys.set_code(hid::page::keyboard_keypad::KEYBOARD_RIGHT_CONTROL, true);
    }
    if (report->modifiers & HID_KEYBOARD_MODIFIER_RIGHTSHIFT) {
        keys.set_code(hid::page::keyboard_keypad::KEYBOARD_RIGHT_SHIFT, true);
    }
    if (report->modifiers & HID_KEYBOARD_MODIFIER_RIGHTGUI) {
        keys.set_code(hid::page::keyboard_keypad::KEYBOARD_RIGHT_GUI, true);
    }
    if (report->modifiers & HID_KEYBOARD_MODIFIER_RIGHTALT) {
        keys.set_code(hid::page::keyboard_keypad::KEYBOARD_RIGHT_ALT, true);
    }
    if (report->modifiers & HID_KEYBOARD_MODIFIER_LEFTCTRL) {
        keys.set_code(hid::page::keyboard_keypad::KEYBOARD_LEFT_CONTROL, true);
    }
    if (report->modifiers & HID_KEYBOARD_MODIFIER_LEFTSHIFT) {
        keys.set_code(hid::page::keyboard_keypad::KEYBOARD_LEFT_SHIFT, true);
    }
    if (report->modifiers & HID_KEYBOARD_MODIFIER_LEFTGUI) {
        keys.set_code(hid::page::keyboard_keypad::KEYBOARD_LEFT_GUI, true);
    }
    if (report->modifiers & HID_KEYBOARD_MODIFIER_LEFTALT) {
        keys.set_code(hid::page::keyboard_keypad::KEYBOARD_LEFT_ALT, true);
    }

    UsbBasicKeyboard_ForeachScancode(report, &UsbCompatibility_KeyboardAddScancode);

    keyboard_app::handle().set_report_state(keys);
}

extern "C" void UsbCompatibility_SendMouseReport(usb_mouse_report_t* report) 
{
    mouse_app::handle().set_report_state(*reinterpret_cast<mouse_buffer*>(report));
}

extern "C" void UsbCompatibility_ConsumerKeyboardAddScancode(uint8_t scancode) 
{
    controls.system_codes.set(static_cast<hid::page::generic_desktop>(scancode), true);
}

extern "C" void UsbCompatibility_SendConsumerReport(usb_media_keyboard_report_t* mediaReport, usb_system_keyboard_report_t* systemReport)
{
    controls = controls_buffer();
    for(uint8_t i = 0; i < USB_MEDIA_KEYBOARD_MAX_KEYS && mediaReport->scancodes[i] != 0; i++) {
        controls.consumer_codes.set(static_cast<hid::page::consumer>(mediaReport->scancodes[i]), true);
    }
    UsbSystemKeyboard_ForeachScancode(systemReport, &UsbCompatibility_ConsumerKeyboardAddScancode);

    controls_app::handle().set_report_state(controls);
}
