#ifndef __USB_COMPATIBILITY_HEADER__
#define __USB_COMPATIBILITY_HEADER__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "usb_interfaces/usb_interface_basic_keyboard.h"
    #include "usb_interfaces/usb_interface_media_keyboard.h"
    #include "usb_interfaces/usb_interface_system_keyboard.h"
    #include "usb_interfaces/usb_interface_mouse.h"

// Macros:

// Typedefs:

    typedef struct {
        bool capsLock;
        bool numLock;
        bool scrollLock;
    } ATTR_PACKED keyboard_led_state_t;

// Variables:

    extern keyboard_led_state_t KeyboardLedsState;

// Functions:

    void UsbCompatibility_SendKeyboardReport(const usb_basic_keyboard_report_t* report);
    void UsbCompatibility_SendMouseReport(const usb_mouse_report_t* report) ;
    void UsbCompatibility_SendConsumerReport(const usb_media_keyboard_report_t* mediaReport, const usb_system_keyboard_report_t* systemReport);
    void UsbCompatibility_SendConsumerReport2(const uint8_t* report);
    void UsbCompatibility_SetKeyboardLedsState(bool capsLock, bool numLock, bool scrollLock);
    bool UsbCompatibility_UsbConnected();

#endif // __USB_HEADER__
