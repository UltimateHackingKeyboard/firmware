#ifndef __USB_INTERFACE_MOUSE_H__
#define __USB_INTERFACE_MOUSE_H__

// Includes:

    #include "usb_device_config.h"
    #include "usb.h"
    #include "usb_device.h"
    #include "include/usb/usb_device_class.h"
    #include "include/usb/usb_device_hid.h"
    #include "include/usb/usb_device_ch9.h"

// Macros:

    #define USB_MOUSE_CLASS (0x03U)
    #define USB_MOUSE_SUBCLASS (0x01U)
    #define USB_MOUSE_PROTOCOL (0x02U)

    #define USB_MOUSE_INTERFACE_INDEX (2U)
    #define USB_MOUSE_INTERFACE_COUNT (1U)
    #define USB_MOUSE_INTERFACE_ALTERNATE_SETTING (0U)

    #define USB_MOUSE_ENDPOINT_ID (1U)
    #define USB_MOUSE_ENDPOINT_COUNT (1U)

    #define USB_MOUSE_INTERRUPT_IN_PACKET_SIZE (8U)
    #define USB_MOUSE_INTERRUPT_IN_INTERVAL (0x04U)

    #define USB_MOUSE_REPORT_LENGTH (0x07U)

// Typedefs:

    typedef struct {
        uint8_t buttons;
        int16_t x;
        int16_t y;
        int8_t wheelX;
        int8_t wheelY;
    } __attribute__ ((packed)) usb_mouse_report_t;

// Variables:

    extern usb_device_class_struct_t UsbMouseClass;

// Functions:

    extern usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param);
    extern usb_status_t UsbMouseSetConfiguration(class_handle_t handle, uint8_t configuration);
    extern usb_status_t UsbMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

#endif
