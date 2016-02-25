#ifndef __USB_DEVICE_HID_MOUSE_H__
#define __USB_DEVICE_HID_MOUSE_H__

// Typedefs:

    typedef struct usb_device_hid_mouse_struct {
        uint8_t buffer[USB_MOUSE_REPORT_LENGTH];
        uint8_t idleRate;
    } usb_device_hid_mouse_struct_t;

// Functions:

    extern usb_status_t UsbMouseInit(usb_device_composite_struct_t *deviceComposite);
    extern usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbMouseSetConfigure(class_handle_t handle, uint8_t configure);
    extern usb_status_t UsbMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
