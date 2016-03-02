#ifndef __USB_DEVICE_HID_MOUSE_H__
#define __USB_DEVICE_HID_MOUSE_H__

// Macros:

    #define USB_MOUSE_REPORT_LENGTH (0x07U)

// Typedefs:

    typedef struct usb_mouse_report {
        uint8_t buttons;
        int16_t x;
        int16_t y;
        int8_t wheelX;
        int8_t wheelY;
    } __attribute__ ((packed)) usb_mouse_report_t;

// Functions:

    extern usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbMouseSetConfiguration(class_handle_t handle, uint8_t configuration);
    extern usb_status_t UsbMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
