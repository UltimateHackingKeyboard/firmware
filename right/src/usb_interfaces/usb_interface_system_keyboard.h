#ifndef __USB_INTERFACE_SYSTEM_KEYBOARD_H__
#define __USB_INTERFACE_SYSTEM_KEYBOARD_H__

// Includes:

    #include "fsl_common.h"
    #include "attributes.h"
    #include "usb_api.h"
    #include "usb_descriptors/usb_descriptor_system_keyboard_report.h"

// Macros:

    #define USB_SYSTEM_KEYBOARD_INTERFACE_INDEX 4
    #define USB_SYSTEM_KEYBOARD_INTERFACE_COUNT 1

    #define USB_SYSTEM_KEYBOARD_ENDPOINT_INDEX 6
    #define USB_SYSTEM_KEYBOARD_ENDPOINT_COUNT 1

    #define USB_SYSTEM_KEYBOARD_INTERRUPT_IN_PACKET_SIZE 1
    #define USB_SYSTEM_KEYBOARD_INTERRUPT_IN_INTERVAL 4

    #define USB_SYSTEM_KEYBOARD_REPORT_LENGTH 1

// Typedefs:

    typedef struct {
        uint8_t scancodes[USB_SYSTEM_KEYBOARD_MAX_KEYS];
    } ATTR_PACKED usb_system_keyboard_report_t;

// Variables:

    extern bool IsUsbSystemKeyboardReportSent;
    extern usb_device_class_struct_t UsbSystemKeyboardClass;
    extern usb_system_keyboard_report_t* ActiveUsbSystemKeyboardReport;

// Functions:

    usb_status_t UsbSystemKeyboardCallback(class_handle_t handle, uint32_t event, void *param);
    usb_status_t UsbSystemKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration);
    usb_status_t UsbSystemKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

    void ResetActiveUsbSystemKeyboardReport(void);
    void SwitchActiveUsbSystemKeyboardReport(void);

#endif
