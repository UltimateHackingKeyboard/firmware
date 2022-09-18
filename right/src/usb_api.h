#ifndef __USB_API_H__
#define __USB_API_H__

// Includes:

    #include <stdbool.h>
    #include "usb.h"
    #include "usb_device.h"
    #include "ksdk_usb/usb_device_class.h"
    #include "ksdk_usb/usb_device_hid.h"

    #include "lufa/Common.h"
    #include "lufa/HIDClassCommon.h"

// Macros:

    #define PACKED(X) X __packed
    #define USB_ALIGNMENT               __attribute__((aligned(4))) // required by USB DMA engine
    #define USB_DESC_STORAGE_TYPE(T)    const T USB_ALIGNMENT

    // General constants

    #define USB_DESCRIPTOR_TYPE_BINARY_OBJECT_STORE (0x0FU)
    #define USB_DESCRIPTOR_TYPE_DEVICE_CAPABILITY   (0x10U)

    #define USB_DEVICE_CLASS 0x00
    #define USB_DEVICE_SUBCLASS 0x00
    #define USB_DEVICE_PROTOCOL 0x00
    #define USB_IRQ_ID USB0_IRQn

    #define USB_INTERFACE_ALTERNATE_SETTING_NONE 0x00
    #define USB_STRING_DESCRIPTOR_NONE           0x00
    #define USB_LANGUAGE_ID_UNITED_STATES        0x0409

    //#define USBD_MS_OS_DESC_VERSION 2

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
    #define HID_RI_USAGE_GENERIC_DESKTOP_Z                     0x32
    #define HID_RI_USAGE_GENERIC_DESKTOP_RX                    0x33
    #define HID_RI_USAGE_GENERIC_DESKTOP_RY                    0x34
    #define HID_RI_USAGE_GENERIC_DESKTOP_RZ                    0x35
    #define HID_RI_USAGE_GENERIC_DESKTOP_WHEEL                 0x38
    #define HID_RI_USAGE_GENERIC_DESKTOP_HAT_SWITCH            0x39
    #define HID_RI_USAGE_GENERIC_DESKTOP_RESOLUTION_MULTIPLIER 0x48
    #define HID_RI_USAGE_GENERIC_DESKTOP_SYSTEM_CONTROL        0x80
    #define HID_RI_USAGE_CONSUMER_CONTROL                      0x01
    #define HID_RI_USAGE_CONSUMER_AC_PAN                       0x0238

    #define HID_RI_COLLECTION_PHYSICAL    0x00
    #define HID_RI_COLLECTION_APPLICATION 0x01
    #define HID_RI_COLLECTION_LOGICAL     0x02

// Functions:
    static inline bool test_bit(unsigned nr, const uint8_t *addr)
    {
        const uint8_t *p = addr;
        return ((1UL << (nr & 7)) & (p[nr >> 3])) != 0;
    }
    static inline void set_bit(unsigned nr, uint8_t *addr)
    {
        uint8_t *p = (uint8_t *)addr;
        p[nr >> 3] |= (1UL << (nr & 7));
    }
    static inline void clear_bit(unsigned nr, uint8_t *addr)
    {
        uint8_t *p = (uint8_t *)addr;
        p[nr >> 3] &= ~(1UL << (nr & 7));
    }

#endif
