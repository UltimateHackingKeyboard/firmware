#ifndef __USB_COMPOSITE_DEVICE_H__
#define __USB_COMPOSITE_DEVICE_H__

// Includes:

    #include "usb_descriptors/usb_descriptor_configuration.h"
    #include "usb_device_config.h"
    #include "usb_device.h"

// Macros:

    #define CONTROLLER_ID kUSB_ControllerKhci0
    #define USB_DEVICE_INTERRUPT_PRIORITY 3

// Typedefs:

    typedef struct {
        usb_device_handle deviceHandle;
        class_handle_t mouseHandle;
        class_handle_t basicKeyboardHandle;
        class_handle_t genericHidHandle;
        class_handle_t mediaKeyboardHandle;
        class_handle_t systemKeyboardHandle;
        uint8_t attach;
        uint8_t currentConfiguration;
        uint8_t currentInterfaceAlternateSetting[USB_DEVICE_CONFIG_HID];
    } usb_composite_device_t;

// Variables:

    extern usb_composite_device_t UsbCompositeDevice;

//Functions:

    void InitUsb(void);

#endif
