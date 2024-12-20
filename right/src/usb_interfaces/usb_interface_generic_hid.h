#ifndef __USB_INTERFACE_GENERIC_HID_H__
#define __USB_INTERFACE_GENERIC_HID_H__

// Includes:

#ifndef __ZEPHYR__
    #include "usb_api.h"
    #include "usb_descriptors/usb_descriptor_device.h"
#endif

// Macros:

    #define USB_GENERIC_HID_INTERFACE_INDEX 4
    #define USB_GENERIC_HID_INTERFACE_COUNT 1

    #define USB_GENERIC_HID_ENDPOINT_IN_INDEX 5
    #define USB_GENERIC_HID_ENDPOINT_OUT_INDEX 6
    #define USB_GENERIC_HID_ENDPOINT_COUNT 2

    #define USB_GENERIC_HID_INTERRUPT_IN_PACKET_SIZE 63
    #define USB_GENERIC_HID_INTERRUPT_IN_INTERVAL 1
    #define USB_GENERIC_HID_INTERRUPT_OUT_PACKET_SIZE 63
    #define USB_GENERIC_HID_INTERRUPT_OUT_INTERVAL 1

    #define USB_GENERIC_HID_IN_BUFFER_LENGTH 63
    #define USB_GENERIC_HID_OUT_BUFFER_LENGTH 63

// Variables:

    extern uint32_t UsbGenericHidActionCounter;

// Functions:

#ifndef __ZEPHYR__
    usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param);
    usb_status_t UsbGenericHidCheckIdleElapsed();
    usb_status_t UsbGenericHidCheckReportReady();
#endif

#endif
