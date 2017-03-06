#include "usb_api.h"
#include "usb_descriptor_system_keyboard_report.h"

uint8_t UsbSystemKeyboardReportDescriptor[USB_SYSTEM_KEYBOARD_REPORT_DESCRIPTOR_LENGTH] = {
    //TODO limit to system keys only?
    //  System Control (Power Down, Sleep, Wakeup, ...)
    0x05, 0x01,                             // USAGE_PAGE (Generic Desktop)
    0x09, 0x80,                             // USAGE (System Control)
    0xa1, 0x01,                             // COLLECTION (Application)
    // 1 system key
    0x15, 0x00,                             // LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x00,                       // LOGICAL_MAXIMUM (255)
    0x19, 0x00,                             // USAGE_MINIMUM (Undefined)
    0x29, 0xff,                             // USAGE_MAXIMUM (System Menu Down)
    0x95, 0x01,                             // REPORT_COUNT (1)
    0x75, 0x08,                             // REPORT_SIZE (8)
    0x81, 0x00,                             // INPUT (Data,Ary,Abs)
    0xc0                                    // END_COLLECTION
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
