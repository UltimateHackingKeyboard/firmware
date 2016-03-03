#ifndef __USB_DESCRIPTOR_CONFIGURATION_H__
#define __USB_DESCRIPTOR_CONFIGURATION_H__

// Includes:

    #include "usb_interface_keyboard.h"
    #include "usb_interface_mouse.h"
    #include "usb_interface_generic_hid.h"

// Macros:

    #define USB_COMPOSITE_CONFIGURATION_INDEX (1U)
    #define USB_LANGUAGE_ID_UNITED_STATES (0x0409U)

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
