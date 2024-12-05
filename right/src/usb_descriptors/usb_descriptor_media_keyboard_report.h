#ifndef __USB_DESCRIPTOR_MEDIA_KEYBOARD_REPORT_H__
#define __USB_DESCRIPTOR_MEDIA_KEYBOARD_REPORT_H__

// Includes:

    #include "usb_api.h"

// Macros:

    #define USB_MEDIA_KEYBOARD_REPORT_DESCRIPTOR_LENGTH (sizeof(UsbMediaKeyboardReportDescriptor))
    #define USB_MEDIA_KEYBOARD_MAX_KEYS 4
    #define HID_CONSUMER_MAX_USAGE  0x03FF

// Variables:

#ifndef __ZEPHYR__
    static USB_DESC_STORAGE_TYPE(uint8_t) UsbMediaKeyboardReportDescriptor[] = {
        HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_CONSUMER),
        HID_RI_USAGE(8, HID_RI_USAGE_CONSUMER_CONTROL),
        HID_RI_COLLECTION(8, HID_RI_COLLECTION_APPLICATION),
            // Media Keys
            HID_RI_LOGICAL_MINIMUM(8, 0),
            HID_RI_LOGICAL_MAXIMUM(16, HID_CONSUMER_MAX_USAGE),
            HID_RI_USAGE_MINIMUM(8, 0),
            HID_RI_USAGE_MAXIMUM(16, HID_CONSUMER_MAX_USAGE),
            HID_RI_REPORT_COUNT(8, USB_MEDIA_KEYBOARD_MAX_KEYS),
            HID_RI_REPORT_SIZE(8, 16),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE),
        HID_RI_END_COLLECTION(0),
    };
#endif

#endif
