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

// Functions:

#ifndef __ZEPHYR__
    usb_status_t USB_DeviceGetHidDescriptor(
        usb_device_handle handle, usb_device_get_hid_descriptor_struct_t *hidDescriptor);

    usb_status_t USB_DeviceGetConfigurationDescriptor(
        usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor, uint8_t msAltEnumCode);
#endif

#endif
