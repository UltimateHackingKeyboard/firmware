#ifndef __USB_INTERFACE_SYSTEM_KEYBOARD_H__
#define __USB_INTERFACE_SYSTEM_KEYBOARD_H__

// Includes:

    #include "usb_descriptors/usb_descriptor_system_keyboard_report.h"
    #include "attributes.h"
    #include "usb_api.h"

// Macros:

    #define USB_SYSTEM_KEYBOARD_INTERFACE_INDEX 3
    #define USB_SYSTEM_KEYBOARD_INTERFACE_COUNT 1

    #define USB_SYSTEM_KEYBOARD_ENDPOINT_INDEX 4
    #define USB_SYSTEM_KEYBOARD_ENDPOINT_COUNT 1

    #define USB_SYSTEM_KEYBOARD_INTERRUPT_IN_PACKET_SIZE (sizeof(usb_system_keyboard_report_t))
    #define USB_SYSTEM_KEYBOARD_INTERRUPT_IN_INTERVAL 1

    #define USB_SYSTEM_KEYBOARD_IS_IN_BITFIELD(scancode) (((scancode) >= USB_SYSTEM_KEYBOARD_MIN_BITFIELD_SCANCODE) && ((scancode) <= USB_SYSTEM_KEYBOARD_MAX_BITFIELD_SCANCODE))

// Typedefs:

    typedef struct {
        uint8_t bitfield[USB_SYSTEM_KEYBOARD_REPORT_LENGTH];
    } ATTR_PACKED usb_system_keyboard_report_t;

// Variables:

    extern uint32_t UsbSystemKeyboardActionCounter;
    extern usb_system_keyboard_report_t* ActiveUsbSystemKeyboardReport;

// Functions:

    bool UsbSystemKeyboard_AddScancode(usb_system_keyboard_report_t* report, uint8_t scancode);
    bool UsbSystemKeyboard_ContainsScancode(const usb_system_keyboard_report_t* report, uint8_t scancode);
    static inline bool UsbSystemKeyboard_UsedScancode(uint8_t scancode)
    {
        return (scancode >= USB_SYSTEM_KEYBOARD_MIN_BITFIELD_SCANCODE) &&
               (scancode <= USB_SYSTEM_KEYBOARD_MAX_BITFIELD_SCANCODE);
    }
#ifndef __ZEPHYR__
    usb_status_t UsbSystemKeyboardCallback(class_handle_t handle, uint32_t event, void *param);

    usb_status_t UsbSystemKeyboardAction(void);
    void UsbSystemKeyboardSendActiveReport(void);

#endif

    void UsbSystemKeyboardResetActiveReport(void);
    void SwitchActiveUsbSystemKeyboardReport(void);
    usb_status_t UsbSystemKeyboardCheckIdleElapsed();
    usb_status_t UsbSystemKeyboardCheckReportReady(bool resending);
    void UsbSystemKeyboard_ForeachScancode(const usb_system_keyboard_report_t* report, void(*action)(uint8_t));
    void UsbSystemKeyboard_RemoveScancode(usb_system_keyboard_report_t* report, uint8_t scancode);
    void UsbSystemKeyboard_MergeReports(const usb_system_keyboard_report_t* sourceReport, usb_system_keyboard_report_t* targetReport);

#endif
