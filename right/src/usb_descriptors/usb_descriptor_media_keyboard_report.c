#include "usb_api.h"
#include "usb_descriptor_media_keyboard_report.h"

uint8_t UsbMediaKeyboardReportDescriptor[USB_MEDIA_KEYBOARD_REPORT_DESCRIPTOR_LENGTH] = {
    // Consumer Control (Sound/Media keys)
    0x05, 0x0C,                                 // usage page (consumer device)
    0x09, 0x01,                                 // usage -- consumer control
    0xA1, 0x01,                                 // collection (application)
    // 4 Media Keys
    0x15, 0x00,                                 // logical minimum
    0x26, 0xFF, 0x03,                           // logical maximum (3ff)
    0x19, 0x00,                                 // usage minimum (0)
    0x2A, 0xFF, 0x03,                           // usage maximum (3ff)
    0x95, 0x04,                                 // report count (4)
    0x75, 0x10,                                 // report size (16)
    0x81, 0x00,                                 // input
    0xC0 // end collection
};

/*
uint8_t UsbMediaKeyboardReportDescriptor[USB_MEDIA_KEYBOARD_REPORT_DESCRIPTOR_LENGTH] = {
    HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_CONSUMER),
    HID_RI_USAGE(8, 0x01),
    HID_RI_COLLECTION(8, HID_RI_COLLECTION_APPLICATION),
        // Scancodes
        HID_RI_LOGICAL_MINIMUM(8, 0x00),
        HID_RI_LOGICAL_MAXIMUM(8, 0xFF),
        HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_KEY_CODES),
        HID_RI_USAGE_MINIMUM(8, 0x00),
        HID_RI_USAGE_MAXIMUM(8, 0xFF),
        HID_RI_REPORT_COUNT(8, USB_MEDIA_KEYBOARD_MAX_KEYS),
        HID_RI_REPORT_SIZE(8, 0x08),
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE),

    HID_RI_END_COLLECTION(0),
};
*/
