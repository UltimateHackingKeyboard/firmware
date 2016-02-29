#include <stdint.h>
#include "usb_report_item_macros.h"
#include "usb_descriptor_keyboard_report.h"

uint8_t UsbKeyboardReportDescriptor[USB_KEYBOARD_REPORT_DESCRIPTOR_LENGTH] = {
    HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
    0x09U, 0x06U, // Usage (Keyboard)
    0xA1U, 0x01U, // Collection (Application)
    0x75U, 0x01U, // Report Size (1U)
    0x95U, 0x08U, // Report Count (8U)
    0x05U, 0x07U, // Usage Page (Key Codes)
    0x19U, 0xE0U, // Usage Minimum (224U)
    0x29U, 0xE7U, // Usage Maximum (231U)

    0x15U, 0x00U, // Logical Minimum (0U)
    0x25U, 0x01U, // Logical Maximum (1U)
    0x81U, 0x02U, // Input (Data, Variable, Absolute) modifier byte

    0x95U, 0x01U, // Report count (1U)
    0x75U, 0x08U, // Report Size (8U)
    0x81U, 0x01U, // Input (Constant), reserved byte
    0x95U, 0x05U, // Report count (5U)
    0x75U, 0x01U, // Report Size (1U)

    0x05U, 0x01U, // Usage Page (Page# for LEDs)
    0x19U, 0x01U, // Usage Minimum (1U)
    0x29U, 0x05U, // Usage Maximum (5U)
    0x91U, 0x02U, // Output (Data, Variable, Absolute) LED report
    0x95U, 0x01U, // Report count (1U)
    0x75U, 0x03U, // Report Size (3U)
    0x91U, 0x01U, // Output (Constant), LED report padding

    0x95U, 0x06U, // Report count (6U)
    0x75U, 0x08U, // Report Size (8U)
    0x15U, 0x00U, // Logical Minimum (0U)
    0x25U, 0xFFU, // Logical Maximum (255U)
    0x05U, 0x07U, // Usage Page (Key Codes)
    0x19U, 0x00U, // Usage Minimum (0U)
    0x29U, 0xFFU, // Usage Maximum (255U)

    0x81U, 0x00U, // Input (Data, Array), Key arrays (6U bytes)
    0xC0U,        // End collection
};
/*
#define HID_DESCRIPTOR_KEYBOARD(MaxKeys)            \
    //HID_RI_USAGE_PAGE(8, 0x01),                     \
    HID_RI_USAGE(8, 0x06),                          \
    HID_RI_COLLECTION(8, 0x01),                     \
        HID_RI_USAGE_PAGE(8, 0x07),                 \
        HID_RI_USAGE_MINIMUM(8, 0xE0),              \
        HID_RI_USAGE_MAXIMUM(8, 0xE7),              \
        HID_RI_LOGICAL_MINIMUM(8, 0x00),            \
        HID_RI_LOGICAL_MAXIMUM(8, 0x01),            \
        HID_RI_REPORT_SIZE(8, 0x01),                \
        HID_RI_REPORT_COUNT(8, 0x08),               \
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE), \
        HID_RI_REPORT_COUNT(8, 0x01),               \
        HID_RI_REPORT_SIZE(8, 0x08),                \
        HID_RI_INPUT(8, HID_IOF_CONSTANT),          \
        HID_RI_USAGE_PAGE(8, 0x08),                 \
        HID_RI_USAGE_MINIMUM(8, 0x01),              \
        HID_RI_USAGE_MAXIMUM(8, 0x05),              \
        HID_RI_REPORT_COUNT(8, 0x05),               \
        HID_RI_REPORT_SIZE(8, 0x01),                \
        HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE | HID_IOF_NON_VOLATILE), \
        HID_RI_REPORT_COUNT(8, 0x01),               \
        HID_RI_REPORT_SIZE(8, 0x03),                \
        HID_RI_OUTPUT(8, HID_IOF_CONSTANT),         \
        HID_RI_LOGICAL_MINIMUM(8, 0x00),            \
        HID_RI_LOGICAL_MAXIMUM(8, 0xFF),            \
        HID_RI_USAGE_PAGE(8, 0x07),                 \
        HID_RI_USAGE_MINIMUM(8, 0x00),              \
        HID_RI_USAGE_MAXIMUM(8, 0xFF),              \
        HID_RI_REPORT_COUNT(8, MaxKeys),            \
        HID_RI_REPORT_SIZE(8, 0x08),                \
        HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE), \
    HID_RI_END_COLLECTION(0)
*/
