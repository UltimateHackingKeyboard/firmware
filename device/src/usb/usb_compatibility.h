#ifndef __USB_COMPATIBILITY_HEADER__
#define __USB_COMPATIBILITY_HEADER__

#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"

void UsbCompatibility_KeyboardAddScancode(uint8_t scancode);
void UsbCompatibility_SendKeyboardReport(usb_basic_keyboard_report_t* report);

#endif // __USB_HEADER__
