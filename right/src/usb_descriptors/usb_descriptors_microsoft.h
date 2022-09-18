#ifndef __USB_DESCRIPTORS_MICROSOFT_H__
#define __USB_DESCRIPTORS_MICROSOFT_H__

// Includes:

    #include "usb.h"

// Functions:

    usb_status_t USB_DeviceGetBosDescriptor(
        usb_device_handle handle, usb_device_get_descriptor_common_struct_t *descriptor);
    usb_status_t USB_DeviceGetMsOsDescriptor(
        usb_device_handle handle, usb_device_control_request_struct_t *controlRequest);

#endif
