#ifndef __USB_COMPATIBILITY_HEADER__
#define __USB_COMPATIBILITY_HEADER__

#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"

void UsbCompatibility_SendKeyboardReport(const usb_basic_keyboard_report_t* report);
void UsbCompatibility_SendMouseReport(const usb_mouse_report_t* report) ;
void UsbCompatibility_SendConsumerReport(const usb_media_keyboard_report_t* mediaReport, const usb_system_keyboard_report_t* systemReport);
void UsbCompatibility_SendConsumerReport2(const uint8_t* report);
bool UsbCompatibility_UsbConnected();

#endif // __USB_HEADER__
