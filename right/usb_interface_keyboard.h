#ifndef __USB_DEVICE_HID_KEYBOARD_H__
#define __USB_DEVICE_HID_KEYBOARD_H__

// Includes:

    #include "usb_descriptor_keyboard_report.h"

// Macros:

    #define USB_KEYBOARD_REPORT_LENGTH (0x08U)

// Typedefs:

    typedef struct usb_keyboard_report {
        uint8_t modifiers;
        uint8_t reserved; // Always must be 0
        uint8_t scancodes[USB_KEYBOARD_MAX_KEYS];
    } __attribute__ ((packed)) usb_keyboard_report_t;

// Functions:

    extern usb_status_t UsbKeyboardCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration);
    extern usb_status_t UsbKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
