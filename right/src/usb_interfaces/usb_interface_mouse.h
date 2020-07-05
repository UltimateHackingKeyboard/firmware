#ifndef __USB_INTERFACE_MOUSE_H__
#define __USB_INTERFACE_MOUSE_H__

// Includes:

    #include "usb_api.h"
    #include "usb_descriptors/usb_descriptor_device.h"

// Macros:

    #define USB_MOUSE_INTERFACE_INDEX 4
    #define USB_MOUSE_INTERFACE_COUNT 1

    #define USB_MOUSE_ENDPOINT_INDEX 6
    #define USB_MOUSE_ENDPOINT_COUNT 1

    #define USB_MOUSE_INTERRUPT_IN_PACKET_SIZE 8
    #define USB_MOUSE_INTERRUPT_IN_INTERVAL 1

    #define USB_MOUSE_REPORT_LENGTH 7

// Typedefs:

    typedef struct {
        uint8_t buttons;
        int16_t x;
        int16_t y;
        int8_t wheelY;
        int8_t wheelX;
    } ATTR_PACKED usb_mouse_report_t;

// Variables:

    extern uint32_t UsbMouseActionCounter;
    extern usb_mouse_report_t* ActiveUsbMouseReport;

// Functions:

    usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param);
    usb_status_t UsbMouseSetConfiguration(class_handle_t handle, uint8_t configuration);
    usb_status_t UsbMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

    void ResetActiveUsbMouseReport(void);
    usb_mouse_report_t* GetInactiveUsbMouseReport(void);
    usb_status_t UsbMouseAction(void);

#endif
