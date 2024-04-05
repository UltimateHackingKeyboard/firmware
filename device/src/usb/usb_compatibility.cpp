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

static scancode_buffer keys;

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
