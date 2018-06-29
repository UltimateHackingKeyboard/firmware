#ifndef __USB_DESCRIPTOR_CONFIGURATION_H__
#define __USB_DESCRIPTOR_CONFIGURATION_H__

// Includes:

    #include "usb_interfaces/usb_interface_basic_keyboard.h"
    #include "usb_interfaces/usb_interface_media_keyboard.h"
    #include "usb_interfaces/usb_interface_system_keyboard.h"
    #include "usb_interfaces/usb_interface_mouse.h"
    #include "usb_interfaces/usb_interface_generic_hid.h"

// Macros:

    #define USB_COMPOSITE_CONFIGURATION_INDEX 1
    #define USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH \
    (5 * USB_DESCRIPTOR_LENGTH_INTERFACE + 5 * USB_DESCRIPTOR_LENGTH_HID + \
    6 * USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_CONFIGURE)

// Variables:

    extern uint8_t UsbConfigurationDescriptor[USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH];

// Functions:

    usb_status_t USB_DeviceGetConfigurationDescriptor(
        usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor);

#endif
