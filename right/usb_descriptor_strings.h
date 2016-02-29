#ifndef __USB_DESCRIPTOR_STRINGS_H__
#define __USB_DESCRIPTOR_STRINGS_H__

// Macros:

    #define USB_LANGUAGE_ID_UNITED_STATES (0x0409U)

    #define USB_STRING_DESCRIPTOR_COUNT (3U)

    #define USB_LANGUAGE_LIST_STRING_DESCRIPTOR_LENGTH (4U)
    #define USB_MANUFACTURER_STRING_DESCRIPTOR_LENGTH (58U)
    #define USB_PRODUCT_STRING_DESCRIPTOR_LENGTH (34U)

    #define USB_STRING_DESCRIPTOR_ID_SUPPORTED_LANGUAGES 0U
    #define USB_STRING_DESCRIPTOR_ID_MANUFACTURER        1U
    #define USB_STRING_DESCRIPTOR_ID_PRODUCT             2U

// Functions:

    extern usb_status_t USB_DeviceGetStringDescriptor(
        usb_device_handle handle, usb_device_get_string_descriptor_struct_t *stringDescriptor);

#endif
