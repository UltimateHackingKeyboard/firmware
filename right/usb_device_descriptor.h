#ifndef __USB_DEVICE_DESCRIPTOR_H__
#define __USB_DEVICE_DESCRIPTOR_H__

// Includes:

    #include "keyboard_descriptors.h"

// Macros:

    #define USB_DEVICE_SPECIFIC_BCD_VERSION (0x0200U)
    #define USB_DEVICE_DEMO_BCD_VERSION (0x0101U)

    #define USB_DEVICE_CLASS (0x00U)
    #define USB_DEVICE_SUBCLASS (0x00U)
    #define USB_DEVICE_PROTOCOL (0x00U)

    #define USB_DEVICE_MAX_POWER (0x32U)

    #define USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL (59U)
    #define USB_DESCRIPTOR_LENGTH_HID_MOUSE_REPORT (52U)
    #define USB_DESCRIPTOR_LENGTH_HID (9U)
    #define USB_DESCRIPTOR_LENGTH_STRING0 (4U)
    #define USB_DESCRIPTOR_LENGTH_STRING1 (58U)
    #define USB_DESCRIPTOR_LENGTH_STRING2 (34U)
    #define USB_DESCRIPTOR_LENGTH_STRING3 (34U)

    #define USB_DEVICE_CONFIGURATION_COUNT (1U)
    #define USB_DEVICE_STRING_COUNT (5U)
    #define USB_DEVICE_LANGUAGE_COUNT (1U)

    #define USB_COMPOSITE_CONFIGURE_INDEX (1U)

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

    #define USB_COMPOSITE_INTERFACE_COUNT (USB_HID_KEYBOARD_INTERFACE_COUNT + USB_HID_MOUSE_INTERFACE_COUNT)

// Function prototypes:

    usb_status_t USB_DeviceGetDeviceDescriptor(
        usb_device_handle handle, usb_device_get_device_descriptor_struct_t *deviceDescriptor);

    usb_status_t USB_DeviceGetConfigurationDescriptor(
        usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor);

    usb_status_t USB_DeviceGetStringDescriptor(
        usb_device_handle handle, usb_device_get_string_descriptor_struct_t *stringDescriptor);

    usb_status_t USB_DeviceGetHidDescriptor(
        usb_device_handle handle, usb_device_get_hid_descriptor_struct_t *hidDescriptor);

    usb_status_t USB_DeviceGetHidReportDescriptor(
        usb_device_handle handle, usb_device_get_hid_report_descriptor_struct_t *hidReportDescriptor);

    usb_status_t USB_DeviceGetHidPhysicalDescriptor(
        usb_device_handle handle, usb_device_get_hid_physical_descriptor_struct_t *hidPhysicalDescriptor);

#endif
