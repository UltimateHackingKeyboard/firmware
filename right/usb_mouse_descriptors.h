#ifndef __USB_MOUSE_DESCRIPTORS_H__
#define __USB_MOUSE_DESCRIPTORS_H__

// Macros:

    #define USB_HID_MOUSE_INTERFACE_COUNT (1U)
    #define USB_HID_MOUSE_INTERFACE_INDEX (0U)
    #define USB_HID_MOUSE_IN_BUFFER_LENGTH (8U)
    #define USB_HID_MOUSE_ENDPOINT_COUNT (1U)
    #define USB_HID_MOUSE_ENDPOINT_IN (1U)

    #define USB_HID_MOUSE_REPORT_LENGTH (0x04U)

    #define USB_HID_MOUSE_CLASS (0x03U)
    #define USB_HID_MOUSE_SUBCLASS (0x01U)
    #define USB_HID_MOUSE_PROTOCOL (0x02U)

    #define FS_HID_MOUSE_INTERRUPT_IN_PACKET_SIZE (8U)
    #define FS_HID_MOUSE_INTERRUPT_IN_INTERVAL (0x04U)

    #define USB_DESCRIPTOR_LENGTH_HID_MOUSE_REPORT (52U)
    #define USB_DESCRIPTOR_LENGTH_STRING3 (34U)

// Variables:

extern usb_device_class_struct_t g_UsbDeviceHidMouseConfig;
extern uint8_t g_UsbDeviceHidMouseReportDescriptor[USB_DESCRIPTOR_LENGTH_HID_MOUSE_REPORT];
extern uint8_t g_UsbDeviceString3[USB_DESCRIPTOR_LENGTH_STRING3];

#endif
