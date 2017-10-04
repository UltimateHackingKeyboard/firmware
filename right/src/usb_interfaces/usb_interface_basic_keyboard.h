#ifndef __USB_INTERFACE_BASIC_KEYBOARD_H__
#define __USB_INTERFACE_BASIC_KEYBOARD_H__

// Includes:

    #include "fsl_common.h"
    #include "attributes.h"
    #include "usb_api.h"
    #include "usb_descriptors/usb_descriptor_basic_keyboard_report.h"

// Macros:

    #define USB_BASIC_KEYBOARD_INTERFACE_INDEX 1
    #define USB_BASIC_KEYBOARD_INTERFACE_COUNT 1

    #define USB_BASIC_KEYBOARD_ENDPOINT_INDEX 3
    #define USB_BASIC_KEYBOARD_ENDPOINT_COUNT 1

    #define USB_BASIC_KEYBOARD_INTERRUPT_IN_PACKET_SIZE 8
    #define USB_BASIC_KEYBOARD_INTERRUPT_IN_INTERVAL 4

    #define USB_BASIC_KEYBOARD_REPORT_LENGTH 8

// Typedefs:

    typedef struct {
        uint8_t modifiers;
        uint8_t reserved; // Always must be 0
        uint8_t scancodes[USB_BASIC_KEYBOARD_MAX_KEYS];
    } ATTR_PACKED usb_basic_keyboard_report_t;

// Variables:

    extern bool IsUsbBasicKeyboardReportSent;
    extern usb_device_class_struct_t UsbBasicKeyboardClass;
    extern usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport;

// Functions:

    usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param);
    usb_status_t UsbBasicKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration);
    usb_status_t UsbBasicKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting);

    void ResetActiveUsbBasicKeyboardReport(void);
    void SwitchActiveUsbBasicKeyboardReport(void);

#endif
