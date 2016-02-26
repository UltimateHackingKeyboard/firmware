#ifndef __USB_DEVICE_COMPOSITE_H__
#define __USB_DEVICE_COMPOSITE_H__

// Macros:

    #define CONTROLLER_ID kUSB_ControllerKhci0
    #define USB_DEVICE_INTERRUPT_PRIORITY (3U)

// Typedefs:

    typedef struct _usb_device_composite_struct {
        usb_device_handle deviceHandle;
        class_handle_t mouseHandle;
        class_handle_t keyboardHandle;
        uint8_t speed;
        uint8_t attach;
        uint8_t currentConfiguration;
        uint8_t currentInterfaceAlternateSetting[USB_COMPOSITE_INTERFACE_COUNT];
    } usb_device_composite_struct_t;

// Variables:

    extern usb_device_composite_struct_t UsbCompositeDevice;

#endif
