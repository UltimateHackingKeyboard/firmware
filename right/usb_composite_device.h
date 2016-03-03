#ifndef __USB_COMPOSITE_DEVICE_H__
#define __USB_COMPOSITE_DEVICE_H__

// Includes:

    #include "usb_descriptor_configuration.h"

// Macros:

    #define CONTROLLER_ID kUSB_ControllerKhci0
    #define USB_DEVICE_INTERRUPT_PRIORITY 3

// Typedefs:

    typedef struct {
        usb_device_handle deviceHandle;
        class_handle_t mouseHandle;
        class_handle_t keyboardHandle;
        class_handle_t genericHidHandle;
        uint8_t attach;
        uint8_t currentConfiguration;
        uint8_t currentInterfaceAlternateSetting[USB_DEVICE_CONFIG_HID];
    } usb_composite_device_t;

// Variables:

    extern usb_composite_device_t UsbCompositeDevice;

//Functions:

    extern void USB_DeviceApplicationInit();

#endif
