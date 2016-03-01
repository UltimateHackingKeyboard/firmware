#ifndef __USB_DESCRIPTOR_KEYBOARD_REPORT_H__
#define __USB_DESCRIPTOR_KEYBOARD_REPORT_H__

// Macros:

    #define USB_KEYBOARD_REPORT_DESCRIPTOR_LENGTH (63U)
    #define USB_KEYBOARD_MAX_KEYS (6U)

// Variables:

    extern uint8_t UsbKeyboardReportDescriptor[USB_KEYBOARD_REPORT_DESCRIPTOR_LENGTH];

#endif
