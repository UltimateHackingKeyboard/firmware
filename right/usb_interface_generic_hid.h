#ifndef __HID_GENERIC_H__
#define __HID_GENERIC_H__

// Macros:

    #define USB_GENERIC_HID_IN_BUFFER_LENGTH (64U)
    #define USB_GENERIC_HID_OUT_BUFFER_LENGTH (64U)

// Functions:

    extern usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbGenericHidSetConfiguration(class_handle_t handle, uint8_t configuration);
    extern usb_status_t UsbGenericHidSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
