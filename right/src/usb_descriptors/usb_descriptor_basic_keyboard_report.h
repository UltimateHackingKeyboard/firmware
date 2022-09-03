#ifndef __USB_DESCRIPTOR_BASIC_KEYBOARD_REPORT_H__
#define __USB_DESCRIPTOR_BASIC_KEYBOARD_REPORT_H__

// Includes:

    #include "usb_api.h"
    #include "lufa/HIDClassCommon.h"

// Macros:

    #define USB_BASIC_KEYBOARD_REPORT_DESCRIPTOR_LENGTH (sizeof(UsbBasicKeyboardReportDescriptor))

    #define USB_BASIC_KEYBOARD_MIN_MODIFIERS_SCANCODE   (HID_KEYBOARD_SC_LEFT_CONTROL)
    #define USB_BASIC_KEYBOARD_MAX_MODIFIERS_SCANCODE   (HID_KEYBOARD_SC_RIGHT_GUI)
    #if (USB_BASIC_KEYBOARD_MAX_MODIFIERS_SCANCODE - USB_BASIC_KEYBOARD_MIN_MODIFIERS_SCANCODE + 1) != 8
        #error USB_BASIC_KEYBOARD modifiers range invalid, adjust report layout and report descriptor
    #endif

    // the first 4 codes are error codes, so not needed in bitmask
    #define USB_BASIC_KEYBOARD_MIN_BITFIELD_SCANCODE    (HID_KEYBOARD_SC_A)
    // this bitfield goes up all the way until the modifiers
    #define USB_BASIC_KEYBOARD_MAX_BITFIELD_SCANCODE    (USB_BASIC_KEYBOARD_MIN_MODIFIERS_SCANCODE - 1)

    #define USB_BASIC_KEYBOARD_BITFIELD_COUNT           (USB_BASIC_KEYBOARD_MAX_BITFIELD_SCANCODE - USB_BASIC_KEYBOARD_MIN_BITFIELD_SCANCODE + 1)
    #define USB_BASIC_KEYBOARD_BITFIELD_LENGTH          ((USB_BASIC_KEYBOARD_BITFIELD_COUNT + 7) / 8)

    #if USB_BASIC_KEYBOARD_MAX_BITFIELD_SCANCODE < 0x65
        #warning USB_BASIC_KEYBOARD_MAX_BITFIELD_SCANCODE less than maximum standard keyboard scancodes
    #endif

// Variables:

    static USB_DESC_STORAGE_TYPE UsbBasicKeyboardReportDescriptor[] = {
        HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_GENERIC_DESKTOP),
        HID_RI_USAGE(8, HID_RI_USAGE_GENERIC_DESKTOP_KEYBOARD),
        HID_RI_COLLECTION(8, HID_RI_COLLECTION_APPLICATION),

            // Modifiers
            HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_KEY_CODES),
            HID_RI_USAGE_MINIMUM(8, USB_BASIC_KEYBOARD_MIN_MODIFIERS_SCANCODE),
            HID_RI_USAGE_MAXIMUM(8, USB_BASIC_KEYBOARD_MAX_MODIFIERS_SCANCODE),
            HID_RI_LOGICAL_MINIMUM(8, 0),
            HID_RI_LOGICAL_MAXIMUM(8, 1),
            HID_RI_REPORT_SIZE(8, 1),
            HID_RI_REPORT_COUNT(8, 8),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

    #if 0 // legacy
            // Reserved for future use, always 0
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_REPORT_SIZE(8, 8),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),

            // Scancodes
            HID_RI_LOGICAL_MINIMUM(8, 0x00),
            HID_RI_LOGICAL_MAXIMUM(16, 0xFF),
            HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_KEY_CODES),
            HID_RI_USAGE_MINIMUM(8, 0x00),
            HID_RI_USAGE_MAXIMUM(8, 0xFF),
            HID_RI_REPORT_COUNT(8, USB_BASIC_KEYBOARD_MAX_KEYS),
            HID_RI_REPORT_SIZE(8, 8),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_ARRAY | HID_IOF_ABSOLUTE),
    #else
            // Scancode Bitmask
            HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_KEY_CODES),
            HID_RI_USAGE_MINIMUM(8, USB_BASIC_KEYBOARD_MIN_BITFIELD_SCANCODE),
            HID_RI_USAGE_MAXIMUM(8, USB_BASIC_KEYBOARD_MAX_BITFIELD_SCANCODE),
            HID_RI_LOGICAL_MINIMUM(8, 0),
            HID_RI_LOGICAL_MAXIMUM(8, 1),
            HID_RI_REPORT_SIZE(8, 1),
            HID_RI_REPORT_COUNT(8, USB_BASIC_KEYBOARD_BITFIELD_COUNT),
            HID_RI_INPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

    #if (USB_BASIC_KEYBOARD_BITFIELD_COUNT % 8 != 0)
            // Padding
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_REPORT_SIZE(8, 8 - (USB_BASIC_KEYBOARD_BITFIELD_COUNT % 8)),
            HID_RI_INPUT(8, HID_IOF_CONSTANT),
    #endif
    #endif

            // LED status - Num lock, Caps lock, Scroll lock, Compose, Kana
            HID_RI_USAGE_PAGE(8, HID_RI_USAGE_PAGE_LEDS),
            HID_RI_USAGE_MINIMUM(8, HID_LED_UID_NUMLOCK),
            HID_RI_USAGE_MAXIMUM(8, HID_LED_UID_KANA),
            HID_RI_REPORT_COUNT(8, 5),
            HID_RI_REPORT_SIZE(8, 1),
            HID_RI_OUTPUT(8, HID_IOF_DATA | HID_IOF_VARIABLE | HID_IOF_ABSOLUTE),

            // LED status padding
            HID_RI_REPORT_COUNT(8, 1),
            HID_RI_REPORT_SIZE(8, 3),
            HID_RI_OUTPUT(8, HID_IOF_CONSTANT),

        HID_RI_END_COLLECTION(0),
    };

#endif
