#ifndef __USB_DESCRIPTOR_BASIC_KEYBOARD_REPORT_H__
#define __USB_DESCRIPTOR_BASIC_KEYBOARD_REPORT_H__

// Macros:

    #define USB_BASIC_KEYBOARD_REPORT_DESCRIPTOR_LENGTH 64
    #define USB_BOOT_KEYBOARD_REPORT_LENGTH 8
    #define USB_BOOT_KEYBOARD_MAX_KEYS 6

    #define USB_BASIC_KEYBOARD_SET_REPORT_LENGTH 1
    // 34 (right) + 31 (left) + 3 (Key module) - 6 (modifiers)
    #define USB_BASIC_KEYBOARD_MAX_KEYS 62 
    #define USB_BASIC_KEYBOARD_REPORT_LENGTH (USB_BASIC_KEYBOARD_MAX_KEYS+2)
    #if USB_BASIC_KEYBOARD_REPORT_LENGTH > 64
        #error USB_BASIC_KEYBOARD_REPORT_LENGTH greater than max usb report length (64)
    #endif

// Variables:

    extern uint8_t UsbBasicKeyboardReportDescriptor[USB_BASIC_KEYBOARD_REPORT_DESCRIPTOR_LENGTH];

#endif
