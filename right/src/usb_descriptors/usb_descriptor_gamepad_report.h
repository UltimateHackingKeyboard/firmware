#ifndef __USB_DESCRIPTOR_GAMEPAD_REPORT_H__
#define __USB_DESCRIPTOR_GAMEPAD_REPORT_H__

// Includes:

    #include "usb_api.h"


// Macros:

    #define USB_GAMEPAD_REPORT_DESCRIPTOR_LENGTH (sizeof(UsbGamepadReportDescriptor))

    #define USB_GAMEPAD_REPORT_IN_PADDING   0

// Variables:
    // replicating the XBOX 360 protocol in HID
    static USB_DESC_STORAGE_TYPE(uint8_t) UsbGamepadReportDescriptor[] = {
        HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
        HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_GAMEPAD),
        HID_RI_COLLECTION(8, HID_RI_COLLECTION_APPLICATION),

            // Report ID (0, which isn't valid, mark it as reserved)
            // Report size
            HID_RI_REPORT_SIZE(8, 8),
            HID_RI_REPORT_COUNT(8, 2),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),

            // Buttons p1
            HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_BUTTONS),
            HID_RI_LOGICAL_MINIMUM(8, 0),
            HID_RI_LOGICAL_MAXIMUM(8, 1),
            HID_RI_REPORT_SIZE(8, 1),
            HID_RI_USAGE(8, 13),
            HID_RI_USAGE(8, 14),
            HID_RI_USAGE(8, 15),
            HID_RI_USAGE(8, 16),
            HID_RI_USAGE(8, 10),
            HID_RI_USAGE(8, 9),
            HID_RI_USAGE(8, 11),
            HID_RI_USAGE(8, 12),
            HID_RI_USAGE(8, 5),
            HID_RI_USAGE(8, 6),
            HID_RI_USAGE(8, 17),
            HID_RI_REPORT_COUNT(8, 11),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

            // 1 bit gap
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),

            // Buttons p2
            HID_RI_USAGE_MINIMUM(8, 1),
            HID_RI_USAGE_MAXIMUM(8, 4),
            HID_RI_REPORT_COUNT(8, 4),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

            // Triggers
            HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
            HID_RI_LOGICAL_MINIMUM(8, 0),
            HID_RI_LOGICAL_MAXIMUM(16, 255),
            HID_RI_REPORT_SIZE(8, 8),
            HID_RI_REPORT_COUNT(8, 1),

            // * Left analog trigger
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            HID_RI_USAGE(32, (HID_RI_USAGE_PAGE_BUTTONS << 16) | 7),
            HID_RI_COLLECTION(8, HID_RI_COLLECTION_PHYSICAL),
#endif
                HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_Z),
                HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            HID_RI_END_COLLECTION(0),
#endif

            // * Right analog trigger
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            HID_RI_USAGE(32, (HID_RI_USAGE_PAGE_BUTTONS << 16) | 8),
            HID_RI_COLLECTION(8, HID_RI_COLLECTION_PHYSICAL),
#endif
                HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_RZ),
                HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            HID_RI_END_COLLECTION(0),
#endif

            // Sticks
            HID_RI_LOGICAL_MINIMUM(16, -32767),
            HID_RI_LOGICAL_MAXIMUM(16, 32767),
            HID_RI_REPORT_SIZE(8, 16),
            HID_RI_REPORT_COUNT(8, 2),

            // * Left stick
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            HID_RI_USAGE(32, (HID_RI_USAGE_PAGE_BUTTONS << 16) | 11),
#endif
            HID_RI_COLLECTION(8, HID_RI_COLLECTION_PHYSICAL),
                HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_X),
                HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_Y),
                HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
            HID_RI_END_COLLECTION(0),

            // * Right stick
#ifdef HID_GAMEPAD_USE_BUTTON_COLLECTIONS
            HID_RI_USAGE(32, (HID_RI_USAGE_PAGE_BUTTONS << 16) | 12),
#endif
            HID_RI_COLLECTION(8, HID_RI_COLLECTION_PHYSICAL),
                HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_RX),
                HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_RY),
                HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),
            HID_RI_END_COLLECTION(0),

#if (USB_GAMEPAD_REPORT_IN_PADDING > 0)
            // Padding
            HID_RI_REPORT_SIZE(8, 8),
            HID_RI_REPORT_COUNT(8, USB_GAMEPAD_REPORT_IN_PADDING),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),
#endif
        HID_RI_END_COLLECTION(0),
    };

#endif
