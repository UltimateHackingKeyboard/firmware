#ifndef __USB_API_H__
#define __USB_API_H__

// Includes:

    #include "usb.h"
    #include "usb_device.h"
    #include "ksdk_usb/usb_device_class.h"
    #include "ksdk_usb/usb_device_hid.h"

    #include "lufa/Common.h"
    #include "lufa/HIDClassCommon.h"

// Macros:

    // General constants

    #define USB_DEVICE_CLASS 0x00
    #define USB_DEVICE_SUBCLASS 0x00
    #define USB_DEVICE_PROTOCOL 0x00

    #define USB_INTERFACE_ALTERNATE_SETTING_NONE 0x00
    #define USB_STRING_DESCRIPTOR_NONE           0x00
    #define USB_LANGUAGE_ID_UNITED_STATES        0x0409

    // HID related constants

    #define USB_DESCRIPTOR_LENGTH_HID 9
    #define USB_CLASS_HID 0x03
    #define USB_HID_COUNTRY_CODE_NOT_SUPPORTED   0x00

    #define USB_HID_SUBCLASS_NONE 0
    #define USB_HID_SUBCLASS_BOOT 1

    #define USB_HID_PROTOCOL_NONE     0
    #define USB_HID_PROTOCOL_KEYBOARD 1
    #define USB_HID_PROTOCOL_MOUSE    2

    // HID report item related constants

    #define HID_RI_USAGE_PAGE_GENERIC_DESKTOP 0x01
    #define HID_RI_USAGE_PAGE_KEY_CODES       0x07
    #define HID_RI_USAGE_PAGE_LEDS            0x08
    #define HID_RI_USAGE_PAGE_BUTTONS         0x09
    #define HID_RI_USAGE_PAGE_CONSUMER        0x0C

    #define HID_RI_USAGE_GENERIC_DESKTOP_POINTER               0x01
    #define HID_RI_USAGE_GENERIC_DESKTOP_MOUSE                 0x02
    #define HID_RI_USAGE_GENERIC_DESKTOP_JOYSTICK              0x04
    #define HID_RI_USAGE_GENERIC_DESKTOP_GAMEPAD               0x05
    #define HID_RI_USAGE_GENERIC_DESKTOP_KEYBOARD              0x06
    #define HID_RI_USAGE_GENERIC_DESKTOP_CONSUMER              0x0C
    #define HID_RI_USAGE_GENERIC_DESKTOP_X                     0x30
    #define HID_RI_USAGE_GENERIC_DESKTOP_Y                     0x31
    #define HID_RI_USAGE_GENERIC_DESKTOP_WHEEL                 0x38
    #define HID_RI_USAGE_GENERIC_DESKTOP_RESOLUTION_MULTIPLIER 0x48
    #define HID_RI_USAGE_GENERIC_DESKTOP_SYSTEM_CONTROL        0x80
    #define HID_RI_USAGE_CONSUMER_CONTROL                      0x01
    #define HID_RI_USAGE_CONSUMER_AC_PAN                       0x0238

    #define HID_RI_COLLECTION_PHYSICAL    0x00
    #define HID_RI_COLLECTION_APPLICATION 0x01
    #define HID_RI_COLLECTION_LOGICAL     0x02

#endif
