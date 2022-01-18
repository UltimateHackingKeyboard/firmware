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
    #define USB_BASIC_KEYBOARD_INTERRUPT_IN_INTERVAL 1

    #define USB_BASIC_KEYBOARD_REPORT_LENGTH 8
    #define USB_BASIC_KEYBOARD_OUT_REPORT_LENGTH 1

    #define USB_BOOT_KEYBOARD_REPORT_LENGTH 8
    #define USB_BOOT_KEYBOARD_MAX_KEYS 6
// Typedefs:

    // Note: We support boot protocol mode in this interface, thus the keyboard
    // report may not exceed 8 bytes and must conform to the HID keyboard boot
    // protocol as specified in the USB HID specification. If a different or
    // longer format is desired in the future, we will need to translate sent
    // reports to the boot protocol format when the host has set boot protocol
    // mode.
    typedef struct {
        uint8_t modifiers;
        uint8_t reserved; // Always must be 0
        uint8_t scancodes[USB_BASIC_KEYBOARD_MAX_KEYS];
    } ATTR_PACKED usb_basic_keyboard_report_t;

// Variables:

    extern uint32_t UsbBasicKeyboardActionCounter;
    extern usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport;

// Functions:

    usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param);

    usb_hid_protocol_t UsbBasicKeyboardGetProtocol(void);
    void UsbBasicKeyboardResetActiveReport(void);
    usb_status_t UsbBasicKeyboardAction(void);
    usb_status_t UsbBasicKeyboardCheckIdleElapsed();
    usb_status_t UsbBasicKeyboardCheckReportReady();

    void UsbBasicKeyboard_AddScancode(usb_basic_keyboard_report_t* report, uint8_t scancode, uint8_t* idx);
    void UsbBasicKeyboard_RemoveScancode(usb_basic_keyboard_report_t* report, uint8_t scancode);
    bool UsbBasicKeyboard_ContainsScancode(const usb_basic_keyboard_report_t* report, uint8_t scancode);
    size_t UsbBasicKeyboard_ScancodeCount(const usb_basic_keyboard_report_t* report);
    void UsbBasicKeyboard_MergeReports(const usb_basic_keyboard_report_t* sourceReport, usb_basic_keyboard_report_t* targetReport, uint8_t* idx);
    void UsbBasicKeyboard_ForeachScancode(const usb_basic_keyboard_report_t* report, void(*action)(uint8_t));

#endif
