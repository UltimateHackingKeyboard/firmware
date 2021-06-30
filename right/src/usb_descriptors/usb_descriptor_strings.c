#include "usb_api.h"
#include "usb_descriptor_strings.h"
#include "device.h"

uint8_t UsbLanguageListStringDescriptor[USB_LANGUAGE_LIST_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbLanguageListStringDescriptor),
    USB_DESCRIPTOR_TYPE_STRING,
    USB_SHORT_GET_LOW(USB_LANGUAGE_ID_UNITED_STATES),
    USB_SHORT_GET_HIGH(USB_LANGUAGE_ID_UNITED_STATES)
};

uint8_t UsbManufacturerString[USB_MANUFACTURER_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbManufacturerString),
    USB_DESCRIPTOR_TYPE_STRING,
    'U', 0x00U,
    'l', 0x00U,
    't', 0x00U,
    'i', 0x00U,
    'm', 0x00U,
    'a', 0x00U,
    't', 0x00U,
    'e', 0x00U,
    ' ', 0x00U,
    'G', 0x00U,
    'a', 0x00U,
    'd', 0x00U,
    'g', 0x00U,
    'e', 0x00U,
    't', 0x00U,
    ' ', 0x00U,
    'L', 0x00U,
    'a', 0x00U,
    'b', 0x00U,
    'o', 0x00U,
    'r', 0x00U,
    'a', 0x00U,
    't', 0x00U,
    'o', 0x00U,
    'r', 0x00U,
    'i', 0x00U,
    'e', 0x00U,
    's', 0x00U,
};

#if DEVICE_ID == DEVICE_ID_UHK60V1

#define USB_PRODUCT_STRING_DESCRIPTOR_LENGTH 20
uint8_t UsbProductString[USB_PRODUCT_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbProductString),
    USB_DESCRIPTOR_TYPE_STRING,
    'U', 0x00U,
    'H', 0x00U,
    'K', 0x00U,
    ' ', 0x00U,
    '6', 0x00U,
    '0', 0x00U,
    ' ', 0x00U,
    'v', 0x00U,
    '1', 0x00U,
};

#elif DEVICE_ID == DEVICE_ID_UHK60V2

#define USB_PRODUCT_STRING_DESCRIPTOR_LENGTH 20
uint8_t UsbProductString[USB_PRODUCT_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbProductString),
    USB_DESCRIPTOR_TYPE_STRING,
    'U', 0x00U,
    'H', 0x00U,
    'K', 0x00U,
    ' ', 0x00U,
    '6', 0x00U,
    '0', 0x00U,
    ' ', 0x00U,
    'v', 0x00U,
    '2', 0x00U,
};

#endif

uint32_t UsbStringDescriptorLengths[USB_STRING_DESCRIPTOR_COUNT] = {
    sizeof(UsbLanguageListStringDescriptor),
    sizeof(UsbManufacturerString),
    sizeof(UsbProductString),
};

uint8_t *UsbStringDescriptors[USB_STRING_DESCRIPTOR_COUNT] = {
    UsbLanguageListStringDescriptor,
    UsbManufacturerString,
    UsbProductString,
};

usb_status_t USB_DeviceGetStringDescriptor(
    usb_device_handle handle, usb_device_get_string_descriptor_struct_t *stringDescriptor)
{
    if (stringDescriptor->stringIndex == 0) {
        stringDescriptor->buffer = UsbLanguageListStringDescriptor;
        stringDescriptor->length = sizeof(UsbLanguageListStringDescriptor);
    } else if (stringDescriptor->languageId == USB_LANGUAGE_ID_UNITED_STATES &&
               stringDescriptor->stringIndex < USB_STRING_DESCRIPTOR_COUNT)
    {
        stringDescriptor->buffer = UsbStringDescriptors[stringDescriptor->stringIndex];
        stringDescriptor->length = UsbStringDescriptorLengths[stringDescriptor->stringIndex];
    } else {
        return kStatus_USB_InvalidRequest;
    }
    return kStatus_USB_Success;
}
