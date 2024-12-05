#ifndef __USB_INTERFACE_MEDIA_KEYBOARD_H__
#define __USB_INTERFACE_MEDIA_KEYBOARD_H__

// Includes:

#ifndef __ZEPHYR__
    #include "fsl_common.h"
#else
    #include "keyboard/legacy_ports.h"
#endif

    #include "usb_descriptors/usb_descriptor_media_keyboard_report.h"
    #include "usb_api.h"

// Macros:

    #define USB_MEDIA_KEYBOARD_INTERFACE_INDEX 2
    #define USB_MEDIA_KEYBOARD_INTERFACE_COUNT 1

    #define USB_MEDIA_KEYBOARD_ENDPOINT_INDEX 3
    #define USB_MEDIA_KEYBOARD_ENDPOINT_COUNT 1

    #define USB_MEDIA_KEYBOARD_INTERRUPT_IN_PACKET_SIZE (USB_MEDIA_KEYBOARD_REPORT_LENGTH)
    #define USB_MEDIA_KEYBOARD_INTERRUPT_IN_INTERVAL 1

    #define USB_MEDIA_KEYBOARD_REPORT_LENGTH (sizeof(usb_media_keyboard_report_t))

// Typedefs:

    typedef struct {
        uint16_t scancodes[USB_MEDIA_KEYBOARD_MAX_KEYS];
    } ATTR_PACKED usb_media_keyboard_report_t;

// Variables:

    extern uint32_t UsbMediaKeyboardActionCounter;
    extern usb_media_keyboard_report_t* ActiveUsbMediaKeyboardReport;

// Functions:

#ifndef __ZEPHYR__
    usb_status_t UsbMediaKeyboardCallback(class_handle_t handle, uint32_t event, void *param);
    usb_status_t UsbMediaKeyboardAction();
#endif

    bool UsbMediaKeyboard_AddScancode(usb_media_keyboard_report_t* report, uint16_t scancode);
    void UsbMediaKeyboardResetActiveReport(void);
    void SwitchActiveUsbMediaKeyboardReport(void);
    usb_status_t UsbMediaKeyboardCheckIdleElapsed();
    usb_status_t UsbMediaKeyboardCheckReportReady();

    void UsbMediaKeyboard_MergeReports(const usb_media_keyboard_report_t* sourceReport, usb_media_keyboard_report_t* targetReport);

#endif
