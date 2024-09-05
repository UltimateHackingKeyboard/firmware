#ifndef __USB_DESCRIPTOR_DEVICE_H__
#define __USB_DESCRIPTOR_DEVICE_H__

// Includes:

    #include "device/device.h"

// Macros:

    #define USB_DEVICE_VENDOR_ID 0x37A8
#if DEVICE_ID == DEVICE_ID_UHK60V1
    #define USB_DEVICE_PRODUCT_ID 0x0001
    #define USB_DEVICE_RELEASE_NUMBER 1
    #define USB_DEVICE_MAX_POWER (150 / 2) // Expressed in 2mA units, 500mA max
#elif DEVICE_ID == DEVICE_ID_UHK60V2
    #define USB_DEVICE_PRODUCT_ID 0x0003
    #define USB_DEVICE_RELEASE_NUMBER 2
    #define USB_DEVICE_MAX_POWER (500 / 2) // Expressed in 2mA units, 500mA max
#endif

    #define USB_DEVICE_SPECIFICATION_VERSION 0x0201
    #define USB_HID_VERSION 0x0110

    #define USB_DEVICE_CONFIGURATION_COUNT 1
    #define USB_REPORT_DESCRIPTOR_COUNT_PER_HID_DEVICE 1

// Functions:

    usb_status_t USB_DeviceGetDeviceDescriptor(
        usb_device_handle handle, usb_device_get_device_descriptor_struct_t *deviceDescriptor);

#endif
