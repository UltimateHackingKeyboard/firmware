#include <stdint.h>
#include "usb_report_item_macros.h"
#include "usb_descriptor_mouse_report.h"
/*
uint8_t UsbMouseReportDescriptor[USB_MOUSE_REPORT_DESCRIPTOR_LENGTH] = {
    0x05U, 0x01U, // Usage Page (Generic Desktop)
    0x09U, 0x02U, // Usage (Mouse)
    0xA1U, 0x01U, // Collection (Application)
    0x09U, 0x01U, // Usage (Pointer)

    0xA1U, 0x00U, // Collection (Physical)
    0x05U, 0x09U, // Usage Page (Buttons)
    0x19U, 0x01U, // Usage Minimum (01U)
    0x29U, 0x03U, // Usage Maximum (03U)

    0x15U, 0x00U, // Logical Minimum (0U)
    0x25U, 0x01U, // Logical Maximum (1U)
    0x95U, 0x03U, // Report Count (3U)
    0x75U, 0x01U, // Report Size (1U)

    0x81U, 0x02U, // Input(Data, Variable, Absolute) 3U button bit fields
    0x95U, 0x01U, // Report count (1U)
    0x75U, 0x05U, // Report Size (5U)
    0x81U, 0x01U, // Input (Constant), 5U constant field

    0x05U, 0x01U, // Usage Page (Generic Desktop)
    0x09U, 0x30U, // Usage (X)
    0x09U, 0x31U, // Usage (Y)
    0x09U, 0x38U, // Usage (Z)

    0x15U, 0x81U, // Logical Minimum (-127)
    0x25U, 0x7FU, // Logical Maximum (127)
    0x75U, 0x08U, // Report Size (8U)
    0x95U, 0x03U, // Report Count (3U)

    0x81U, 0x06U, // Input (Data, Variable, Relative), 3 position bytes (X & Y & Z)
    0xC0U,        // End collection, Close Pointer collection
    0xC0U         // End collection, Close Mouse collection

};
*/

uint8_t UsbMouseReportDescriptor[USB_MOUSE_REPORT_DESCRIPTOR_LENGTH] = {
    HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
    HID_RI_USAGE(8, HID_RI_USAGE_MOUSE),
    HID_RI_COLLECTION(8, HID_RI_COLLECTION_APPLICATION),
        HID_RI_USAGE(8, HID_RI_USAGE_POINTER),
        HID_RI_COLLECTION(8, HID_RI_COLLECTION_PHYSICAL),

            // Mouse buttons
            HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_MOUSE_BUTTONS),
            HID_RI_USAGE_MINIMUM(8, 0x01),
            HID_RI_USAGE_MAXIMUM(8, USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS),
            HID_RI_LOGICAL_MINIMUM(8, 0x00),
            HID_RI_LOGICAL_MAXIMUM(8, 0x01),
            HID_RI_REPORT_COUNT(8, USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS),
            HID_RI_REPORT_SIZE(8, 0x01),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

            // Mouse buttons padding
            HID_RI_REPORT_COUNT(8, 0x01),
            HID_RI_REPORT_SIZE(8, (USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS % 8) ? (8 - (USB_MOUSE_REPORT_DESCRIPTOR_BUTTONS % 8)) : 0),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),

            // Mouse X and Y coordinates
            HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
            HID_RI_USAGE(8, HID_RI_USAGE_X),
            HID_RI_USAGE(8, HID_RI_USAGE_Y),
            HID_RI_LOGICAL_MINIMUM(16, USB_MOUSE_REPORT_DESCRIPTOR_MIN_AXIS_VALUE),
            HID_RI_LOGICAL_MAXIMUM(16, USB_MOUSE_REPORT_DESCRIPTOR_MAX_AXIS_VALUE),
            HID_RI_PHYSICAL_MINIMUM(16, USB_MOUSE_REPORT_DESCRIPTOR_MIN_AXIS_PHYSICAL_VALUE),
            HID_RI_PHYSICAL_MAXIMUM(16, USB_MOUSE_REPORT_DESCRIPTOR_MAX_AXIS_PHYSICAL_VALUE),
            HID_RI_REPORT_COUNT(8, 0x02),
            HID_RI_REPORT_SIZE(8, (((USB_MOUSE_REPORT_DESCRIPTOR_MIN_AXIS_VALUE >= -128) && (USB_MOUSE_REPORT_DESCRIPTOR_MAX_AXIS_VALUE <= 127)) ? 8 : 16)),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_RELATIVE),

            HID_RI_COLLECTION(8, HID_RI_COLLECTION_LOGICAL),

                // Vertical wheel resolution multiplier
                HID_RI_USAGE(8, HID_RI_USAGE_RESOLUTION_MULTIPLIER),
                0x15, 0x00,        //         LOGICAL_MINIMUM (0)
                0x25, 0x01,        //         LOGICAL_MAXIMUM (1)
                0x35, 0x01,        //         PHYSICAL_MINIMUM (1)
                0x45, 0x04,        //         PHYSICAL_MAXIMUM (4)
                0x75, 0x02,        //         REPORT_SIZE (2)
                0x95, 0x01,        //         REPORT_COUNT (1)
                0xa4,              //         PUSH
                0xb1, 0x02,        //         FEATURE (Data,Var,Abs)

                // Vertical wheel
                HID_RI_USAGE(8, HID_RI_USAGE_WHEEL),
                0x15, 0x81,        //         LOGICAL_MINIMUM (-127)
                0x25, 0x7f,        //         LOGICAL_MAXIMUM (127)
                0x35, 0x00,        //         PHYSICAL_MINIMUM (0)        - reset physical
                0x45, 0x00,        //         PHYSICAL_MAXIMUM (0)
                0x75, 0x08,        //         REPORT_SIZE (8)
                0x81, 0x06,        //         INPUT (Data,Var,Rel)
            HID_RI_END_COLLECTION(0),

            HID_RI_COLLECTION(8, HID_RI_COLLECTION_LOGICAL),

                // Horizontal wheel resolution multiplier
                HID_RI_USAGE(8, HID_RI_USAGE_RESOLUTION_MULTIPLIER),
                0xb4,              //         POP
                0xb1, 0x02,        //         FEATURE (Data,Var,Abs)

                // Padding
                0x35, 0x00,        //         PHYSICAL_MINIMUM (0)        - reset physical
                0x45, 0x00,        //         PHYSICAL_MAXIMUM (0)
                0x75, 0x04,        //         REPORT_SIZE (4)
                0xb1, 0x03,        //         FEATURE (Cnst,Var,Abs)

                // Horizontal wheel
                0x05, 0x0c,        //         USAGE_PAGE (Consumer Devices)
                0x0a, 0x38, 0x02,  //         USAGE (AC Pan)
                0x15, 0x81,        //         LOGICAL_MINIMUM (-127)
                0x25, 0x7f,        //         LOGICAL_MAXIMUM (127)
                0x75, 0x08,        //         REPORT_SIZE (8)
                0x81, 0x06,        //         INPUT (Data,Var,Rel)

                HID_RI_END_COLLECTION(0),
        HID_RI_END_COLLECTION(0),
    HID_RI_END_COLLECTION(0)
};
