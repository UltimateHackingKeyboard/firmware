#ifndef __USB_DESCRIPTOR_MOUSE_REPORT_H__
#define __USB_DESCRIPTOR_MOUSE_REPORT_H__

// Includes:

    #include "usb_api.h"

// Macros:

    #define USB_MOUSE_REPORT_DESCRIPTOR_LENGTH (sizeof(UsbMouseReportDescriptor))

    #define USB_MOUSE_REPORT_DESCRIPTOR_MIN_AXIS_VALUE -4096
    #define USB_MOUSE_REPORT_DESCRIPTOR_MAX_AXIS_VALUE 4096
    #define USB_MOUSE_REPORT_DESCRIPTOR_MIN_AXIS_PHYSICAL_VALUE -4096
    #define USB_MOUSE_REPORT_DESCRIPTOR_MAX_AXIS_PHYSICAL_VALUE 4096
    #define USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS 20

    #define USB_MOUSE_REPORT_DESCRIPTOR_MIN_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE 1
    #define USB_MOUSE_REPORT_DESCRIPTOR_MAX_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE 120

    #define USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS_PADDING ((USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS % 8) \
                ? (8 - (USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS % 8)) \
                : 0)

// Variables:

    static USB_DESC_STORAGE_TYPE(uint8_t) UsbMouseReportDescriptor[] = {
        HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
        HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_MOUSE),
        HID_RI_COLLECTION(8, HID_RI_COLLECTION_APPLICATION),
            HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_POINTER),
            HID_RI_COLLECTION(8, HID_RI_COLLECTION_PHYSICAL),

                // Mouse buttons
                HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_BUTTONS),
                HID_RI_USAGE_MINIMUM(8, 1),
                HID_RI_USAGE_MAXIMUM(8, USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS),
                HID_RI_LOGICAL_MINIMUM(8, 0),
                HID_RI_LOGICAL_MAXIMUM(8, 1),
                HID_RI_REPORT_COUNT(8, USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS),
                HID_RI_REPORT_SIZE(8, 1),
                HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

                // Mouse buttons padding
                HID_RI_REPORT_COUNT(8, 1),
                HID_RI_REPORT_SIZE(8, USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS_PADDING),
                HID_RI_INPUT(8, HID_IOF_CONSTANT),

                // Mouse X and Y coordinates
                HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
                HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_X),
                HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_Y),
                HID_RI_LOGICAL_MINIMUM(16, USB_MOUSE_REPORT_DESCRIPTOR_MIN_AXIS_VALUE),
                HID_RI_LOGICAL_MAXIMUM(16, USB_MOUSE_REPORT_DESCRIPTOR_MAX_AXIS_VALUE),
                HID_RI_PHYSICAL_MINIMUM(16, USB_MOUSE_REPORT_DESCRIPTOR_MIN_AXIS_PHYSICAL_VALUE),
                HID_RI_PHYSICAL_MAXIMUM(16, USB_MOUSE_REPORT_DESCRIPTOR_MAX_AXIS_PHYSICAL_VALUE),
                HID_RI_REPORT_COUNT(8, 2),
                HID_RI_REPORT_SIZE(8, 16),
                HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_RELATIVE),

                HID_RI_COLLECTION(8, HID_RI_COLLECTION_LOGICAL),

                    // Scroll wheels

                    // Resolution multiplier for high-res scroll support
                    // To have a multiplier apply to a wheel, it must be in the
                    // same logical collection as the wheel, or else there must
                    // be no logical collections (according to the USB HID spec);
                    // so to have a single multiplier apply to the two wheels,
                    // they must be in the same logical collection (or there
                    // must be no logical collection at all)
                    HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_RESOLUTION_MULTIPLIER),
                    HID_RI_LOGICAL_MINIMUM(8, 0),
                    HID_RI_LOGICAL_MAXIMUM(8, 1),
                    HID_RI_PHYSICAL_MINIMUM(8, USB_MOUSE_REPORT_DESCRIPTOR_MIN_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE),
                    HID_RI_PHYSICAL_MAXIMUM(8, USB_MOUSE_REPORT_DESCRIPTOR_MAX_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE),
                    HID_RI_REPORT_COUNT(8, 1),
                    HID_RI_REPORT_SIZE(8, 8),
                    HID_RI_FEATURE(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

                    // Vertical wheel
                    HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_WHEEL),
                    HID_RI_LOGICAL_MINIMUM(16, -32767),
                    HID_RI_LOGICAL_MAXIMUM(16, 32767),
                    HID_RI_PHYSICAL_MINIMUM(16, -32767),
                    HID_RI_PHYSICAL_MAXIMUM(16, 32767),
                    HID_RI_REPORT_COUNT(8, 1),
                    HID_RI_REPORT_SIZE(8, 16),
                    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_RELATIVE),

                    // Horizontal wheel
                    HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_CONSUMER),
                    HID_RI_USAGE(16, HID_RI_USAGE_CONSUMER_AC_PAN),
                    HID_RI_LOGICAL_MINIMUM(16, -32767),
                    HID_RI_LOGICAL_MAXIMUM(16, 32767),
                    HID_RI_PHYSICAL_MINIMUM(16, -32767),
                    HID_RI_PHYSICAL_MAXIMUM(16, 32767),
                    HID_RI_REPORT_COUNT(8, 1),
                    HID_RI_REPORT_SIZE(8, 16),
                    HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_RELATIVE),

                HID_RI_END_COLLECTION(0),

            HID_RI_END_COLLECTION(0),
        HID_RI_END_COLLECTION(0)
    };

#endif
