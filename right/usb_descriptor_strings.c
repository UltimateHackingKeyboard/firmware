#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "usb_descriptor_strings.h"

uint8_t UsbLanguageListStringDescriptor[USB_LANGUAGE_LIST_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbLanguageListStringDescriptor),
    USB_DESCRIPTOR_TYPE_STRING,
    USB_SHORT_GET_LOW(USB_LANGUAGE_ID_UNITED_STATES),
    USB_SHORT_GET_HIGH(USB_LANGUAGE_ID_UNITED_STATES)
};

uint8_t UsbManufacturerString[USB_MANUFACTURER_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbManufacturerString),
    USB_DESCRIPTOR_TYPE_STRING,
    'F', 0x00U,
    'R', 0x00U,
    'E', 0x00U,
    'E', 0x00U,
    'S', 0x00U,
    'C', 0x00U,
    'A', 0x00U,
    'L', 0x00U,
    'E', 0x00U,
    ' ', 0x00U,
    'S', 0x00U,
    'E', 0x00U,
    'M', 0x00U,
    'I', 0x00U,
    'C', 0x00U,
    'O', 0x00U,
    'N', 0x00U,
    'D', 0x00U,
    'U', 0x00U,
    'C', 0x00U,
    'T', 0x00U,
    'O', 0x00U,
    'R', 0x00U,
    ' ', 0x00U,
    'I', 0x00U,
    'N', 0x00U,
    'C', 0x00U,
    '.', 0x00U,
};

uint8_t UsbProductString[USB_PRODUCT_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbProductString),
    USB_DESCRIPTOR_TYPE_STRING,
    'C', 0x00U,
    'O', 0x00U,
    'M', 0x00U,
    'P', 0x00U,
    'O', 0x00U,
    'S', 0x00U,
    'I', 0x00U,
    'T', 0x00U,
    'E', 0x00U,
    ' ', 0x00U,
    'D', 0x00U,
    'E', 0x00U,
    'V', 0x00U,
    'I', 0x00U,
    'C', 0x00U,
    'E', 0x00U,
};

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
