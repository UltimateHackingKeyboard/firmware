#ifndef __USB_DEVICE_HID_KEYBOARD_H__
#define __USB_DEVICE_HID_KEYBOARD_H__

// Macros:

    #define USB_KEYBOARD_REPORT_LENGTH (0x08U)

// Typedefs:

    typedef struct _usb_device_hid_keyboard_struct {
        uint8_t buffer[USB_KEYBOARD_REPORT_LENGTH];
        uint8_t idleRate;
    } usb_device_hid_keyboard_struct_t;

// Functions:

    extern usb_status_t UsbKeyboardInit(usb_device_composite_struct_t *compositeDevice);
    extern usb_status_t UsbKeyboardCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbKeyboardSetConfigure(class_handle_t handle, uint8_t configuration);
    extern usb_status_t UsbKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
