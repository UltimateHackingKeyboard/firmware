#ifndef __USB_CLASS_KEYBOARD_H__
#define __USB_CLASS_KEYBOARD_H__

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

// Variables:

    extern usb_device_class_struct_t UsbKeyboardClass;

#endif
