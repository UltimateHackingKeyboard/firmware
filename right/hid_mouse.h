#ifndef __USB_DEVICE_HID_MOUSE_H__
#define __USB_DEVICE_HID_MOUSE_H__

/* Type defines: */

    typedef struct usb_device_hid_mouse_struct {
        uint8_t buffer[USB_HID_MOUSE_REPORT_LENGTH];
        uint8_t idleRate;
    } usb_device_hid_mouse_struct_t;

/* Function prototypes: */

    extern usb_status_t USB_DeviceHidMouseInit(usb_device_composite_struct_t *deviceComposite);
    extern usb_status_t USB_DeviceHidMouseCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t USB_DeviceHidMouseSetConfigure(class_handle_t handle, uint8_t configure);
    extern usb_status_t USB_DeviceHidMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
