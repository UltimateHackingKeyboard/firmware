#ifndef __USB_DESCRIPTOR_CONFIGURATION_H__
#define __USB_DESCRIPTOR_CONFIGURATION_H__

// Includes:

    #include "usb_interface_keyboard.h"
    #include "usb_interface_mouse.h"
    #include "usb_interface_generic_hid.h"

// Macros:

    #define USB_DEVICE_CLASS (0x00U)
    #define USB_DEVICE_SUBCLASS (0x00U)
    #define USB_DEVICE_PROTOCOL (0x00U)

    #define USB_DEVICE_SPECIFICATION_VERSION (0x0200U)
    #define USB_HID_VERSION (0x0100U)

    #define USB_DEVICE_VENDOR_ID (0x15A2U)
    #define USB_DEVICE_PRODUCT_ID (0x007EU)
    #define USB_DEVICE_RELEASE_NUMBER (0x0101U)

    #define USB_COMPOSITE_CONFIGURATION_INDEX (1U)
    #define USB_DEVICE_CONFIGURATION_COUNT (1U)
    #define USB_REPORT_DESCRIPTOR_COUNT_PER_HID_DEVICE (1U)
    #define USB_DEVICE_MAX_POWER (50U) // Expressed in 2mA units
    #define USB_COMPOSITE_INTERFACE_COUNT (USB_KEYBOARD_INTERFACE_COUNT + \
                                           USB_MOUSE_INTERFACE_COUNT + \
                                           USB_GENERIC_HID_INTERFACE_COUNT)

    #define USB_LANGUAGE_ID_UNITED_STATES (0x0409U)
    #define USB_HID_COUNTRY_CODE_NOT_SUPPORTED (0x00U)
    #define USB_INTERFACE_ALTERNATE_SETTING_NONE (0x00U)
    #define USB_STRING_DESCRIPTOR_NONE             0U

    // Descriptor lengths

    #define USB_HID_DESCRIPTOR_LENGTH (9U)
    #define USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH (91U)

// Functions:

    extern usb_status_t USB_DeviceGetConfigurationDescriptor(
        usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor);

    extern usb_status_t USB_DeviceGetHidDescriptor(
        usb_device_handle handle, usb_device_get_hid_descriptor_struct_t *hidDescriptor);

    extern usb_status_t USB_DeviceGetHidReportDescriptor(
        usb_device_handle handle, usb_device_get_hid_report_descriptor_struct_t *hidReportDescriptor);

    extern usb_status_t USB_DeviceGetHidPhysicalDescriptor(
        usb_device_handle handle, usb_device_get_hid_physical_descriptor_struct_t *hidPhysicalDescriptor);

#endif
