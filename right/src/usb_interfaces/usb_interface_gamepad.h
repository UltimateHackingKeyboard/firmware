#ifndef __USB_INTERFACE_GAMEPAD_H__
#define __USB_INTERFACE_GAMEPAD_H__

// Includes:

    #include "usb_api.h"

// Macros:

    #define USB_GAMEPAD_INTERFACE_INDEX 5
    #define USB_GAMEPAD_INTERFACE_COUNT 1

    #define USB_GAMEPAD_ENDPOINT_INDEX 7
    #define USB_GAMEPAD_ENDPOINT_COUNT 2

    #define USB_GAMEPAD_INTERRUPT_IN_PACKET_SIZE (USB_GAMEPAD_REPORT_LENGTH)
    #define USB_GAMEPAD_INTERRUPT_IN_INTERVAL 1

    #define USB_GAMEPAD_INTERRUPT_OUT_PACKET_SIZE 8
    #define USB_GAMEPAD_INTERRUPT_OUT_INTERVAL 0xFF

    #define USB_GAMEPAD_REPORT_LENGTH (sizeof(usb_gamepad_report_t))

// Typedefs:
    typedef enum {
        _GAMEPAD_BUTTONS_BEGIN = 0,
        GAMEPAD_A = 0,
        GAMEPAD_B = 1,
        GAMEPAD_X = 2,
        GAMEPAD_Y = 3,
        GAMEPAD_LEFT_BUMPER = 4,
        GAMEPAD_RIGHT_BUMPER = 5,
        GAMEPAD_BACK = 8,
        GAMEPAD_START = 9,
        GAMEPAD_LEFT_STICK_PRESS = 10,
        GAMEPAD_RIGHT_STICK_PRESS = 11,
        GAMEPAD_DPAD_UP = 12,
        GAMEPAD_DPAD_DOWN = 13,
        GAMEPAD_DPAD_LEFT = 14,
        GAMEPAD_DPAD_RIGHT = 15,
        GAMEPAD_HOME = 16,
        _GAMEPAD_BUTTONS_END = GAMEPAD_HOME,

        GAMEPAD_LEFT_TRIGGER_ANALOG = 30,  // button 6
        GAMEPAD_RIGHT_TRIGGER_ANALOG = 31, // button 7

        GAMEPAD_LEFT_STICK_X = 32,
        GAMEPAD_LEFT_STICK_Y = 33,
        GAMEPAD_RIGHT_STICK_X = 34,
        GAMEPAD_RIGHT_STICK_Y = 35,
    } usb_gamepad_property_t;

    typedef PACKED(union) {
        PACKED(struct) {
            uint8_t reportId;
            uint8_t reportSize;
            union {
                struct {
                    uint16_t UP : 1;
                    uint16_t DOWN : 1;
                    uint16_t LEFT : 1;
                    uint16_t RIGHT : 1;
                    uint16_t START : 1;
                    uint16_t BACK : 1;
                    uint16_t LS : 1;
                    uint16_t RS : 1;
                    uint16_t LB : 1;
                    uint16_t RB : 1;
                    uint16_t HOME : 1;
                    uint16_t : 1;
                    uint16_t A : 1;
                    uint16_t B : 1;
                    uint16_t X : 1;
                    uint16_t Y : 1;
                };
                uint16_t buttons;
            };
            uint8_t lTrigger;
            uint8_t rTrigger;
            int16_t lX;
            int16_t lY;
            int16_t rX;
            int16_t rY;
#if (USB_GAMEPAD_REPORT_IN_PADDING > 0)
            uint8_t reserved[USB_GAMEPAD_REPORT_IN_PADDING];
#endif
        } X360;
    } usb_gamepad_report_t;

// Variables:

    extern uint32_t UsbGamepadActionCounter;
    extern usb_gamepad_report_t* ActiveUsbGamepadReport;

// Functions:

    usb_status_t UsbGamepadCallback(class_handle_t handle, uint32_t event, void *param);

    void UsbGamepadResetActiveReport(void);
    usb_status_t UsbGamepadAction(void);
    usb_status_t UsbGamepadCheckIdleElapsed();
    usb_status_t UsbGamepadCheckReportReady();

    uint16_t UsbGamepad_GetPropertyMask(usb_gamepad_property_t key);

    void UsbGamepadSetProperty(usb_gamepad_report_t* report, usb_gamepad_property_t key, int value);
    void UsbGamepad_MergeReports(const usb_gamepad_report_t* sourceReport, usb_gamepad_report_t* targetReport);

#endif
