#ifndef __USB_DEVICE_HID_KEYBOARD_H__
#define __USB_DEVICE_HID_KEYBOARD_H__

// Includes:

    #include "usb_device_config.h"
    #include "usb.h"
    #include "usb_device.h"
    #include "include/usb/usb_device_class.h"
    #include "include/usb/usb_device_hid.h"
    #include "include/usb/usb_device_ch9.h"
    #include "usb_descriptor_keyboard_report.h"

// Macros:

    #define USB_KEYBOARD_CLASS (0x03U)
    #define USB_KEYBOARD_SUBCLASS (0x01U)
    #define USB_KEYBOARD_PROTOCOL (0x01U)

    #define USB_KEYBOARD_INTERFACE_INDEX (1U)
    #define USB_KEYBOARD_INTERFACE_COUNT (1U)
    #define USB_KEYBOARD_INTERFACE_ALTERNATE_SETTING (0U)

    #define USB_KEYBOARD_ENDPOINT_ID (2U)
    #define USB_KEYBOARD_ENDPOINT_COUNT (1U)

    #define USB_KEYBOARD_INTERRUPT_IN_PACKET_SIZE (8U)
    #define USB_KEYBOARD_INTERRUPT_IN_INTERVAL (0x04U)

    #define USB_KEYBOARD_REPORT_LENGTH (0x08U)

// Typedefs:

    typedef struct usb_keyboard_report {
        uint8_t modifiers;
        uint8_t reserved; // Always must be 0
        uint8_t scancodes[USB_KEYBOARD_MAX_KEYS];
    } __attribute__ ((packed)) usb_keyboard_report_t;

// Variables:
    extern usb_device_class_struct_t UsbKeyboardClass;

// Functions:

    extern usb_status_t UsbKeyboardCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration);
    extern usb_status_t UsbKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
