#ifndef __USB_INTERFACE_BASIC_KEYBOARD_H__
#define __USB_INTERFACE_BASIC_KEYBOARD_H__

// Includes:

    #include <stddef.h>
    #include <stdint.h>

    #include "attributes.h"
    #include "usb_api.h"
    #include "usb_descriptors/usb_descriptor_basic_keyboard_report.h"

// Macros:


    #define USB_BASIC_KEYBOARD_INTERFACE_INDEX 0
    #define USB_BASIC_KEYBOARD_INTERFACE_COUNT 1

    #define USB_BASIC_KEYBOARD_ENDPOINT_INDEX 1
    #define USB_BASIC_KEYBOARD_ENDPOINT_COUNT 1

    #define USB_BASIC_KEYBOARD_INTERRUPT_IN_PACKET_SIZE (USB_BASIC_KEYBOARD_REPORT_LENGTH)
    #define USB_BASIC_KEYBOARD_INTERRUPT_IN_INTERVAL 1

    #define USB_BASIC_KEYBOARD_REPORT_LENGTH (1 + USB_BASIC_KEYBOARD_BITFIELD_LENGTH)
    #if USB_BASIC_KEYBOARD_REPORT_LENGTH > 64
        #error USB_BASIC_KEYBOARD_REPORT_LENGTH greater than max usb report length (64)
    #endif

    // Technically should be one, but the stack for some reason always writes 4,
    // so we need to dedicate 4 bytes in order to avoid overwriting other data.
    #define USB_BASIC_KEYBOARD_OUT_REPORT_LENGTH 4

    #define USB_BOOT_KEYBOARD_REPORT_LENGTH (2 + USB_BOOT_KEYBOARD_MAX_KEYS)
    #define USB_BOOT_KEYBOARD_MAX_KEYS 6

// Typedefs:

    typedef struct {
        uint8_t modifiers;
        union {
            struct {
                uint8_t reserved; // Always must be 0
                uint8_t scancodes[USB_BOOT_KEYBOARD_MAX_KEYS];
            } ATTR_PACKED boot;
            uint8_t bitfield[USB_BASIC_KEYBOARD_BITFIELD_LENGTH];
        }ATTR_PACKED;
    } ATTR_PACKED usb_basic_keyboard_report_t;

// Variables:
    extern bool UsbBasicKeyboard_CapsLockOn;
    extern bool UsbBasicKeyboard_NumLockOn;
    extern bool UsbBasicKeyboard_ScrollLockOn;
    extern uint32_t UsbBasicKeyboardActionCounter;
    extern usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport;

// Functions:

#ifndef __ZEPHYR__
    usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param);

    usb_status_t UsbBasicKeyboardAction(void);
    usb_status_t UsbBasicKeyboardCheckIdleElapsed();
#endif

    static inline bool UsbBasicKeyboard_IsModifier(uint8_t scancode)
    {
        return (scancode >= USB_BASIC_KEYBOARD_MIN_MODIFIERS_SCANCODE) &&
               (scancode <= USB_BASIC_KEYBOARD_MAX_MODIFIERS_SCANCODE);
    }
    static inline bool UsbBasicKeyboard_IsInBitfield(uint8_t scancode)
    {
        return (scancode >= USB_BASIC_KEYBOARD_MIN_BITFIELD_SCANCODE) &&
               (scancode <= USB_BASIC_KEYBOARD_MAX_BITFIELD_SCANCODE);
    }
    bool UsbBasicKeyboard_IsFullScancodes(const usb_basic_keyboard_report_t* report);
    usb_hid_protocol_t UsbBasicKeyboardGetProtocol(void);
    bool UsbBasicKeyboard_AddScancode(usb_basic_keyboard_report_t* report, uint8_t scancode);
    void UsbBasicKeyboard_RemoveScancode(usb_basic_keyboard_report_t* report, uint8_t scancode);
    bool UsbBasicKeyboard_ContainsScancode(const usb_basic_keyboard_report_t* report, uint8_t scancode);
    size_t UsbBasicKeyboard_ScancodeCount(const usb_basic_keyboard_report_t* report);
    void UsbBasicKeyboard_MergeReports(const usb_basic_keyboard_report_t* sourceReport, usb_basic_keyboard_report_t* targetReport);
    void UsbBasicKeyboard_ForeachScancode(const usb_basic_keyboard_report_t* report, void(*action)(uint8_t));
    void UsbBasicKeyboard_HandleProtocolChange();
    void UsbBasicKeyboardResetActiveReport(void);
    usb_status_t UsbBasicKeyboardCheckReportReady();
    void UsbBasicKeyboardSendActiveReport(void);
    usb_basic_keyboard_report_t* GetInactiveUsbBasicKeyboardReport(void);
    void SwitchActiveUsbBasicKeyboardReport(void);

#endif
