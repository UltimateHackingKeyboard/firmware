#ifndef __USB_CLASS_MOUSE_H__
#define __USB_CLASS_MOUSE_H__

// Macros:

    #define USB_MOUSE_CLASS (0x03U)
    #define USB_MOUSE_SUBCLASS (0x01U)
    #define USB_MOUSE_PROTOCOL (0x02U)

    #define USB_MOUSE_INTERFACE_INDEX (0U)
    #define USB_MOUSE_INTERFACE_COUNT (1U)
    #define USB_MOUSE_INTERFACE_ALTERNATE_SETTING (0U)

    #define USB_MOUSE_ENDPOINT_ID (1U)
    #define USB_MOUSE_ENDPOINT_COUNT (1U)

    #define USB_MOUSE_INTERRUPT_IN_PACKET_SIZE (8U)
    #define USB_MOUSE_INTERRUPT_IN_INTERVAL (0x04U)

// Variables:

    extern usb_device_class_struct_t UsbMouseClass;

#endif
