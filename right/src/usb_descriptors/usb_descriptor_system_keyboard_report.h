#ifndef __USB_DESCRIPTOR_SYSTEM_KEYBOARD_REPORT_H__
#define __USB_DESCRIPTOR_SYSTEM_KEYBOARD_REPORT_H__

// Includes:

    #include "usb_api.h"
    #include "arduino_hid/SystemAPI.h"

// Macros:

    #define USB_SYSTEM_KEYBOARD_REPORT_DESCRIPTOR_LENGTH (sizeof(UsbSystemKeyboardReportDescriptor))
    #define USB_SYSTEM_KEYBOARD_REPORT_LENGTH 1

    #define USB_SYSTEM_KEYBOARD_MIN_BITFIELD_SCANCODE SYSTEM_POWER_DOWN
    #define USB_SYSTEM_KEYBOARD_MAX_BITFIELD_SCANCODE SYSTEM_WAKE_UP

    #if USB_SYSTEM_KEYBOARD_REPORT_LENGTH > 64
        #error USB_SYSTEM_KEYBOARD_REPORT_LENGTH greater than max usb report length (64)
    #endif

// Variables:

#ifndef __ZEPHYR__
    static USB_DESC_STORAGE_TYPE(uint8_t) UsbSystemKeyboardReportDescriptor[] = {
        HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
        HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_SYSTEM_CONTROL),
        HID_RI_COLLECTION(8, HID_RI_COLLECTION_APPLICATION),
            // System keys
            HID_RI_LOGICAL_MINIMUM(8, 0),
            HID_RI_LOGICAL_MAXIMUM(8, 1),
            HID_RI_USAGE_MINIMUM(8, USB_SYSTEM_KEYBOARD_MIN_BITFIELD_SCANCODE),
            HID_RI_USAGE_MAXIMUM(8, USB_SYSTEM_KEYBOARD_MAX_BITFIELD_SCANCODE),
            HID_RI_REPORT_SIZE(8, 1),
            HID_RI_REPORT_COUNT(8, 3),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

            // Padding
            HID_RI_REPORT_SIZE(8, 1),
            HID_RI_REPORT_COUNT(8, 5),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),
        HID_RI_END_COLLECTION(0),
    };
#endif

#endif
