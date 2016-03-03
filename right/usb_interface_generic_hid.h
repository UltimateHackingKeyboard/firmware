#ifndef __USB_INTERFACE_GENERIC_HID_H__
#define __USB_INTERFACE_GENERIC_HID_H__

// Includes:

    #include "usb_device_config.h"
    #include "usb.h"
    #include "usb_device.h"
    #include "include/usb/usb_device_class.h"
    #include "include/usb/usb_device_hid.h"
    #include "usb_descriptor_device.h"

// Macros:

    #define USB_GENERIC_HID_CLASS (0x03U)
    #define USB_GENERIC_HID_SUBCLASS (0x00U)
    #define USB_GENERIC_HID_PROTOCOL (0x00U)

    #define USB_GENERIC_HID_INTERFACE_INDEX (0U)
    #define USB_GENERIC_HID_INTERFACE_COUNT (1U)
    #define USB_GENERIC_HID_INTERFACE_ALTERNATE_SETTING (0U)

    #define USB_GENERIC_HID_ENDPOINT_IN_ID (3U)
    #define USB_GENERIC_HID_ENDPOINT_OUT_ID (4U)
    #define USB_GENERIC_HID_ENDPOINT_COUNT (2U)

    #define USB_GENERIC_HID_INTERRUPT_IN_PACKET_SIZE (64U)
    #define USB_GENERIC_HID_INTERRUPT_IN_INTERVAL (0x04U)
    #define USB_GENERIC_HID_INTERRUPT_OUT_PACKET_SIZE (64U)
    #define USB_GENERIC_HID_INTERRUPT_OUT_INTERVAL (0x04U)

    #define USB_GENERIC_HID_IN_BUFFER_LENGTH (64U)
    #define USB_GENERIC_HID_OUT_BUFFER_LENGTH (64U)

// Variables:

    extern usb_device_class_struct_t UsbGenericHidClass;

// Functions:

    extern usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbGenericHidSetConfiguration(class_handle_t handle, uint8_t configuration);
    extern usb_status_t UsbGenericHidSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
