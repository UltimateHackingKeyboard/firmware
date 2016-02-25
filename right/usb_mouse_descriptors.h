#ifndef __USB_MOUSE_DESCRIPTORS_H__
#define __USB_MOUSE_DESCRIPTORS_H__

// Macros:

    #define USB_MOUSE_CLASS (0x03U)
    #define USB_MOUSE_SUBCLASS (0x01U)
    #define USB_MOUSE_PROTOCOL (0x02U)

    #define USB_MOUSE_INTERFACE_INDEX (0U)
    #define USB_MOUSE_INTERFACE_COUNT (1U)
    #define USB_MOUSE_INTERFACE_ALTERNATE_SETTING (0U)

    #define USB_MOUSE_ENDPOINT_IN (1U)
    #define USB_MOUSE_ENDPOINT_COUNT (1U)

    #define USB_MOUSE_INTERRUPT_IN_PACKET_SIZE (8U)
    #define USB_MOUSE_INTERRUPT_IN_INTERVAL (0x04U)

    #define USB_MOUSE_REPORT_LENGTH (0x04U)
    #define USB_MOUSE_REPORT_DESCRIPTOR_LENGTH (52U)
    #define USB_MOUSE_STRING_DESCRIPTOR_LENGTH (34U)

// Variables:

    extern usb_device_class_struct_t UsbDeviceMouseClass;
    extern uint8_t UsbMouseReportDescriptor[USB_MOUSE_REPORT_DESCRIPTOR_LENGTH];
    extern uint8_t UsbMouseString[USB_MOUSE_STRING_DESCRIPTOR_LENGTH];

#endif
