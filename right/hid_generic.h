#ifndef __HID_GENERIC_H__
#define __HID_GENERIC_H__

// Macros:

    #define USB_GENERIC_HID_IN_BUFFER_LENGTH (64U)
    #define USB_GENERIC_HID_OUT_BUFFER_LENGTH (64U)

// Typedefs:

    typedef struct _usb_device_generic_hid_struct {
        uint32_t buffer[2][USB_GENERIC_HID_IN_BUFFER_LENGTH >> 2];
        uint8_t bufferIndex;
        uint8_t idleRate;
    } usb_device_generic_hid_struct_t;

// Functions:

    extern usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbGenericHidSetConfigure(class_handle_t handle, uint8_t configuration);
    extern usb_status_t UsbGenericHidSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
