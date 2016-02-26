#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "hid_keyboard.h"
#include "hid_mouse.h"

uint8_t UsbDeviceDescriptor[USB_DESCRIPTOR_LENGTH_DEVICE] = {
    USB_DESCRIPTOR_LENGTH_DEVICE,
    USB_DESCRIPTOR_TYPE_DEVICE,
    USB_SHORT_GET_LOW(USB_DEVICE_SPECIFICATION_VERSION),
    USB_SHORT_GET_HIGH(USB_DEVICE_SPECIFICATION_VERSION),
    USB_DEVICE_CLASS,
    USB_DEVICE_SUBCLASS,
    USB_DEVICE_PROTOCOL,
    USB_CONTROL_MAX_PACKET_SIZE,
    USB_SHORT_GET_LOW(USB_DEVICE_VENDOR_ID),
    USB_SHORT_GET_HIGH(USB_DEVICE_VENDOR_ID),
    USB_SHORT_GET_LOW(USB_DEVICE_PRODUCT_ID),
    USB_SHORT_GET_HIGH(USB_DEVICE_PRODUCT_ID),
    USB_SHORT_GET_LOW(USB_DEVICE_RELEASE_NUMBER),
    USB_SHORT_GET_HIGH(USB_DEVICE_RELEASE_NUMBER),
    USB_STRING_DESCRIPTOR_ID_MANUFACTURER,
    USB_STRING_DESCRIPTOR_ID_PRODUCT,
    USB_STRING_DESCRIPTOR_ID_SUPPORTED_LANGUAGES,
    USB_DEVICE_CONFIGURATION_COUNT,
};

uint8_t UsbConfigurationDescriptor[USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH] = {

// Configuration descriptor

    USB_DESCRIPTOR_LENGTH_CONFIGURE,
    USB_DESCRIPTOR_TYPE_CONFIGURE,
    USB_SHORT_GET_LOW(USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH),
    USB_SHORT_GET_HIGH(USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH),
    USB_COMPOSITE_INTERFACE_COUNT,
    USB_COMPOSITE_CONFIGURATION_INDEX,
    USB_STRING_DESCRIPTOR_NONE,
    (USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_D7_MASK) |
        (USB_DEVICE_CONFIG_SELF_POWER << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_SELF_POWERED_SHIFT) |
        (USB_DEVICE_CONFIG_REMOTE_WAKEUP << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_REMOTE_WAKEUP_SHIFT),
    USB_DEVICE_MAX_POWER,

// Mouse interface descriptor

    USB_DESCRIPTOR_LENGTH_INTERFACE,
    USB_DESCRIPTOR_TYPE_INTERFACE,
    USB_MOUSE_INTERFACE_INDEX,
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    USB_MOUSE_ENDPOINT_COUNT,
    USB_MOUSE_CLASS,
    USB_MOUSE_SUBCLASS,
    USB_MOUSE_PROTOCOL,
    USB_STRING_DESCRIPTOR_ID_MOUSE,

// Mouse HID descriptor

    USB_HID_DESCRIPTOR_LENGTH,
    USB_DESCRIPTOR_TYPE_HID,
    USB_SHORT_GET_LOW(USB_HID_VERSION),
    USB_SHORT_GET_HIGH(USB_HID_VERSION),
    USB_HID_COUNTRY_CODE_NOT_SUPPORTED,
    USB_REPORT_DESCRIPTOR_COUNT_PER_HID_DEVICE,
    USB_DESCRIPTOR_TYPE_HID_REPORT,
    USB_SHORT_GET_LOW(USB_MOUSE_REPORT_DESCRIPTOR_LENGTH),
    USB_SHORT_GET_HIGH(USB_MOUSE_REPORT_DESCRIPTOR_LENGTH),

// Mouse endpoint descriptor

    USB_DESCRIPTOR_LENGTH_ENDPOINT,
    USB_DESCRIPTOR_TYPE_ENDPOINT,
    USB_MOUSE_ENDPOINT_ID | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_SHORT_GET_LOW(USB_MOUSE_INTERRUPT_IN_PACKET_SIZE),
    USB_SHORT_GET_HIGH(USB_MOUSE_INTERRUPT_IN_PACKET_SIZE),
    USB_MOUSE_INTERRUPT_IN_INTERVAL,

// Keyboard interface descriptor

    USB_DESCRIPTOR_LENGTH_INTERFACE,
    USB_DESCRIPTOR_TYPE_INTERFACE,
    USB_KEYBOARD_INTERFACE_INDEX,
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    USB_KEYBOARD_ENDPOINT_COUNT,
    USB_KEYBOARD_CLASS,
    USB_KEYBOARD_SUBCLASS,
    USB_KEYBOARD_PROTOCOL,
    USB_STRING_DESCRIPTOR_ID_KEYBOARD,

// Keyboard HID descriptor

    USB_HID_DESCRIPTOR_LENGTH,
    USB_DESCRIPTOR_TYPE_HID,
    USB_SHORT_GET_LOW(USB_HID_VERSION),
    USB_SHORT_GET_HIGH(USB_HID_VERSION),
    USB_HID_COUNTRY_CODE_NOT_SUPPORTED,
    USB_REPORT_DESCRIPTOR_COUNT_PER_HID_DEVICE,
    USB_DESCRIPTOR_TYPE_HID_REPORT,
    USB_SHORT_GET_LOW(USB_KEYBOARD_REPORT_DESCRIPTOR_LENGTH),
    USB_SHORT_GET_HIGH(USB_KEYBOARD_REPORT_DESCRIPTOR_LENGTH),

// Keyboard endpoint descriptor

    USB_DESCRIPTOR_LENGTH_ENDPOINT,
    USB_DESCRIPTOR_TYPE_ENDPOINT,
    USB_KEYBOARD_ENDPOINT_ID | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_SHORT_GET_LOW(USB_KEYBOARD_INTERRUPT_IN_PACKET_SIZE),
    USB_SHORT_GET_HIGH(USB_KEYBOARD_INTERRUPT_IN_PACKET_SIZE),
    USB_KEYBOARD_INTERRUPT_IN_INTERVAL,
};

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
    sizeof(UsbMouseString),
    sizeof(UsbKeyboardString),
};

uint8_t *UsbStringDescriptors[USB_STRING_DESCRIPTOR_COUNT] = {
    UsbLanguageListStringDescriptor,
    UsbManufacturerString,
    UsbProductString,
    UsbMouseString,
    UsbKeyboardString,
};

usb_status_t USB_DeviceGetDeviceDescriptor(
    usb_device_handle handle, usb_device_get_device_descriptor_struct_t *deviceDescriptor)
{
    deviceDescriptor->buffer = UsbDeviceDescriptor;
    deviceDescriptor->length = USB_DESCRIPTOR_LENGTH_DEVICE;
    return kStatus_USB_Success;
}

usb_status_t USB_DeviceGetConfigurationDescriptor(
    usb_device_handle handle, usb_device_get_configuration_descriptor_struct_t *configurationDescriptor)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX > configurationDescriptor->configuration) {
        configurationDescriptor->buffer = UsbConfigurationDescriptor;
        configurationDescriptor->length = USB_CONFIGURATION_DESCRIPTOR_TOTAL_LENGTH;
        return kStatus_USB_Success;
    }
    return kStatus_USB_InvalidRequest;
}

usb_status_t USB_DeviceGetStringDescriptor(
    usb_device_handle handle, usb_device_get_string_descriptor_struct_t *stringDescriptor)
{
    if (stringDescriptor->stringIndex == 0U) {
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

usb_status_t USB_DeviceGetHidDescriptor(
    usb_device_handle handle, usb_device_get_hid_descriptor_struct_t *hidDescriptor)
{
    return kStatus_USB_InvalidRequest;
}

usb_status_t USB_DeviceGetHidReportDescriptor(
    usb_device_handle handle, usb_device_get_hid_report_descriptor_struct_t *hidReportDescriptor)
{
    if (USB_MOUSE_INTERFACE_INDEX == hidReportDescriptor->interfaceNumber) {
        hidReportDescriptor->buffer = UsbMouseReportDescriptor;
        hidReportDescriptor->length = USB_MOUSE_REPORT_DESCRIPTOR_LENGTH;
    } else if (USB_KEYBOARD_INTERFACE_INDEX == hidReportDescriptor->interfaceNumber) {
        hidReportDescriptor->buffer = UsbKeyboardReportDescriptor;
        hidReportDescriptor->length = USB_KEYBOARD_REPORT_DESCRIPTOR_LENGTH;
    } else {
        return kStatus_USB_InvalidRequest;
    }
    return kStatus_USB_Success;
}

usb_status_t USB_DeviceGetHidPhysicalDescriptor(
    usb_device_handle handle, usb_device_get_hid_physical_descriptor_struct_t *hidPhysicalDescriptor)
{
    return kStatus_USB_InvalidRequest;
}
