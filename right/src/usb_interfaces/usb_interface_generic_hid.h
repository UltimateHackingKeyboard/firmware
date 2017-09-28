#ifndef __USB_INTERFACE_GENERIC_HID_H__
#define __USB_INTERFACE_GENERIC_HID_H__

// Includes:

    #include "usb_api.h"
    #include "usb_descriptors/usb_descriptor_device.h"
    #include "usb_protocol_handler.h"

// Macros:

    #define USB_GENERIC_HID_INTERFACE_INDEX 0
    #define USB_GENERIC_HID_INTERFACE_COUNT 1

    #define USB_GENERIC_HID_ENDPOINT_IN_INDEX 1
    #define USB_GENERIC_HID_ENDPOINT_OUT_INDEX 2
    #define USB_GENERIC_HID_ENDPOINT_COUNT 2

    #define USB_GENERIC_HID_INTERRUPT_IN_PACKET_SIZE 64
    #define USB_GENERIC_HID_INTERRUPT_IN_INTERVAL 4
    #define USB_GENERIC_HID_INTERRUPT_OUT_PACKET_SIZE 64
    #define USB_GENERIC_HID_INTERRUPT_OUT_INTERVAL 4

    #define USB_GENERIC_HID_IN_BUFFER_LENGTH 64
    #define USB_GENERIC_HID_OUT_BUFFER_LENGTH 64

// Variables:

    extern usb_device_class_struct_t UsbGenericHidClass;
    extern uint8_t GenericHidInBuffer[USB_GENERIC_HID_IN_BUFFER_LENGTH];
    extern uint8_t GenericHidOutBuffer[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

// Functions:

    usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param);
    usb_status_t UsbGenericHidSetConfiguration(class_handle_t handle, uint8_t configuration);
    usb_status_t UsbGenericHidSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
