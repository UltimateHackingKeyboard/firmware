#ifndef __USB_DESCRIPTOR_STRINGS_H__
#define __USB_DESCRIPTOR_STRINGS_H__

// Includes:

    #include <stdint.h>

// Macros:

    #define USB_STRING_DESCRIPTOR_COUNT 4

    #define USB_LANGUAGE_LIST_STRING_DESCRIPTOR_LENGTH 4
    #define USB_MANUFACTURER_STRING_DESCRIPTOR_LENGTH  58

    #define USB_STRING_DESCRIPTOR_ID_SUPPORTED_LANGUAGES 0
    #define USB_STRING_DESCRIPTOR_ID_MANUFACTURER        1
    #define USB_STRING_DESCRIPTOR_ID_PRODUCT             2
    #define USB_STRING_DESCRIPTOR_ID_SERIAL              3

// Functions:

    usb_status_t USB_DeviceGetStringDescriptor(
        usb_device_handle handle, usb_device_get_string_descriptor_struct_t *stringDescriptor);
    void USB_SetSerialNo(uint32_t serial);

#endif
