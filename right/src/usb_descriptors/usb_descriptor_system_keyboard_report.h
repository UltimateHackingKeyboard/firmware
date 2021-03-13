#ifndef __USB_DESCRIPTOR_SYSTEM_KEYBOARD_REPORT_H__
#define __USB_DESCRIPTOR_SYSTEM_KEYBOARD_REPORT_H__

// Macros:

    #define USB_SYSTEM_KEYBOARD_REPORT_DESCRIPTOR_LENGTH 31
    #define USB_SYSTEM_KEYBOARD_REPORT_LENGTH 1

    // the first 4 codes are error codes, so not needed in bitmask
    #define USB_SYSTEM_KEYBOARD_MIN_BITFIELD_SCANCODE 0x81
    #define USB_SYSTEM_KEYBOARD_MAX_BITFIELD_SCANCODE 0x83
    
    #if USB_SYSTEM_KEYBOARD_REPORT_LENGTH > 64
        #error USB_SYSTEM_KEYBOARD_REPORT_LENGTH greater than max usb report length (64)
    #endif
// Variables:

    extern uint8_t UsbSystemKeyboardReportDescriptor[USB_SYSTEM_KEYBOARD_REPORT_DESCRIPTOR_LENGTH];

#endif
