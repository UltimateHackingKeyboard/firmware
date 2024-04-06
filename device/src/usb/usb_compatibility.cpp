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

extern "C" void UsbCompatibility_SendKeyboardReport(usb_basic_keyboard_report_t* report) 
{
    // the report layout is the same (as long as the report is in NKRO mode)
    keyboard_app::handle().set_report_state(*reinterpret_cast<scancode_buffer*>(report));
}
