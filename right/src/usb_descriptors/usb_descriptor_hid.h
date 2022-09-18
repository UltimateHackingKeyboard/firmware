#ifndef __USB_DESCRIPTOR_HID_H__
#define __USB_DESCRIPTOR_HID_H__

// Includes:

    #include "usb.h"

// Functions:

    usb_status_t USB_DeviceGetHidReportDescriptor(
        usb_device_handle handle, usb_device_get_hid_report_descriptor_struct_t *hidReportDescriptor);

    usb_status_t USB_DeviceGetHidPhysicalDescriptor(
        usb_device_handle handle, usb_device_get_hid_physical_descriptor_struct_t *hidPhysicalDescriptor);

#endif
