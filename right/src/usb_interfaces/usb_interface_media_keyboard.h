#ifndef __USB_INTERFACE_MEDIA_KEYBOARD_H__
#define __USB_INTERFACE_MEDIA_KEYBOARD_H__

// Includes:

    #include "fsl_common.h"
    #include "usb_api.h"
    #include "usb_descriptors/usb_descriptor_media_keyboard_report.h"

// Macros:

    #define USB_MEDIA_KEYBOARD_INTERFACE_INDEX 3
    #define USB_MEDIA_KEYBOARD_INTERFACE_COUNT 1

    #define USB_MEDIA_KEYBOARD_ENDPOINT_INDEX 5
    #define USB_MEDIA_KEYBOARD_ENDPOINT_COUNT 1

    #define USB_MEDIA_KEYBOARD_INTERRUPT_IN_PACKET_SIZE 8
    #define USB_MEDIA_KEYBOARD_INTERRUPT_IN_INTERVAL 4

    #define USB_MEDIA_KEYBOARD_REPORT_LENGTH 8

// Typedefs:

    typedef struct {
        uint16_t scancodes[USB_MEDIA_KEYBOARD_MAX_KEYS];
    } ATTR_PACKED usb_media_keyboard_report_t;

// Variables:

    extern bool IsUsbMediaKeyboardReportSent;
    extern usb_device_class_struct_t UsbMediaKeyboardClass;
    extern usb_media_keyboard_report_t* ActiveUsbMediaKeyboardReport;

// Functions:

    usb_status_t UsbMediaKeyboardCallback(class_handle_t handle, uint32_t event, void *param);
    usb_status_t UsbMediaKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration);
    usb_status_t UsbMediaKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

    void ResetActiveUsbMediaKeyboardReport(void);
    void SwitchActiveUsbMediaKeyboardReport(void);

#endif
