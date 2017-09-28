#ifndef __USB_DESCRIPTOR_DEVICE_H__
#define __USB_DESCRIPTOR_DEVICE_H__

// Macros:

    #define USB_DEVICE_VENDOR_ID 0x1D50
    #define USB_DEVICE_PRODUCT_ID 0x6122
    #define USB_DEVICE_RELEASE_NUMBER 0x0101

    #define USB_DEVICE_SPECIFICATION_VERSION 0x0200
    #define USB_HID_VERSION 0x0110

    #define USB_DEVICE_CONFIGURATION_COUNT 1
    #define USB_REPORT_DESCRIPTOR_COUNT_PER_HID_DEVICE 1
    #define USB_DEVICE_MAX_POWER 50 // Expressed in 2mA units

// Functions:

    usb_status_t USB_DeviceGetDeviceDescriptor(
        usb_device_handle handle, usb_device_get_device_descriptor_struct_t *deviceDescriptor);

#endif
