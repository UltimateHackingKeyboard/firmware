#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "util.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "hid_keyboard.h"
#include "hid_mouse.h"

uint8_t UsbDeviceDescriptor[USB_DESCRIPTOR_LENGTH_DEVICE] = {
    USB_DESCRIPTOR_LENGTH_DEVICE,
    USB_DESCRIPTOR_TYPE_DEVICE,
    USB_SHORT_GET_LOW(USB_DEVICE_SPECIFICATION_BCD_VERSION),
    USB_SHORT_GET_HIGH(USB_DEVICE_SPECIFICATION_BCD_VERSION),
    USB_DEVICE_CLASS,
    USB_DEVICE_SUBCLASS,
    USB_DEVICE_PROTOCOL,
    USB_CONTROL_MAX_PACKET_SIZE,
    GET_LSB_OF_WORD(USB_VENDOR_ID),
    GET_MSB_OF_WORD(USB_VENDOR_ID),
    GET_LSB_OF_WORD(USB_PRODUCT_ID),
    GET_MSB_OF_WORD(USB_PRODUCT_ID),
    USB_SHORT_GET_LOW(USB_DEVICE_RELEASE_NUMBER),
    USB_SHORT_GET_HIGH(USB_DEVICE_RELEASE_NUMBER),
    USB_STRING_DESCRIPTOR_ID_MANUFACTURER,
    USB_STRING_DESCRIPTOR_ID_PRODUCT,
    USB_STRING_DESCRIPTOR_ID_SERIAL_NUMBER,
    USB_DEVICE_CONFIGURATION_COUNT,
};

uint8_t UsbConfigurationDescriptor[USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL] = {

// Configuration descriptor

    USB_DESCRIPTOR_LENGTH_CONFIGURE,
    USB_DESCRIPTOR_TYPE_CONFIGURE,
    USB_SHORT_GET_LOW(USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL),
    USB_SHORT_GET_HIGH(USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL), // Total length of data returned for this configuration.
    USB_COMPOSITE_INTERFACE_COUNT,
    USB_COMPOSITE_CONFIGURE_INDEX, // Value to use as an argument to the SetConfiguration() request to select this configuration
    0x00U, // Index of string descriptor describing this configuration

    // Configuration characteristics
    //     D7: Reserved (set to one)
    //     D6: Self-powered
    //     D5: Remote Wakeup
    //     D4...0: Reserved (reset to zero)
    (USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_D7_MASK) |
        (USB_DEVICE_CONFIG_SELF_POWER << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_SELF_POWERED_SHIFT) |
        (USB_DEVICE_CONFIG_REMOTE_WAKEUP << USB_DESCRIPTOR_CONFIGURE_ATTRIBUTE_REMOTE_WAKEUP_SHIFT),

    // Maximum power consumption of the USB device from the bus in this specific configuration
    // when the device is fully operational. Expressed in 2 mA units (i.e., 50 = 100 mA).
    USB_DEVICE_MAX_POWER,

// Mouse interface descriptor

    USB_DESCRIPTOR_LENGTH_INTERFACE,
    USB_DESCRIPTOR_TYPE_INTERFACE,
    USB_MOUSE_INTERFACE_INDEX,
    0x00U, //  Value used to select this alternate setting for the interface identified in the prior field
    USB_MOUSE_ENDPOINT_COUNT,
    USB_MOUSE_CLASS,
    USB_MOUSE_SUBCLASS,
    USB_MOUSE_PROTOCOL,
    USB_STRING_DESCRIPTOR_ID_MOUSE,

// Mouse HID descriptor

    USB_DESCRIPTOR_LENGTH_HID,
    USB_DESCRIPTOR_TYPE_HID,
    0x00U, 0x01U,                   // ID Class Specification release.
    0x00U,                          // Country code of the localized hardware
    0x01U,                          // Number of class descriptors (at least one report descriptor)
    USB_DESCRIPTOR_TYPE_HID_REPORT,
    USB_SHORT_GET_LOW(USB_DESCRIPTOR_LENGTH_MOUSE_REPORT),
    USB_SHORT_GET_HIGH(USB_DESCRIPTOR_LENGTH_MOUSE_REPORT),

// Mouse endpoint descriptor

    USB_DESCRIPTOR_LENGTH_ENDPOINT,
    USB_DESCRIPTOR_TYPE_ENDPOINT,
    // The address of the endpoint on the USB device described by this descriptor.
    USB_MOUSE_ENDPOINT_IN | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT, // This field describes the endpoint's attributes
    // Maximum packet size this endpoint is capable of sending or receiving when this configuration is selected.
    USB_SHORT_GET_LOW(USB_MOUSE_INTERRUPT_IN_PACKET_SIZE), USB_SHORT_GET_HIGH(USB_MOUSE_INTERRUPT_IN_PACKET_SIZE),
    USB_MOUSE_INTERRUPT_IN_INTERVAL, // Interval for polling endpoint for data transfers.

// Keyboard interface descriptor

    USB_DESCRIPTOR_LENGTH_INTERFACE,
    USB_DESCRIPTOR_TYPE_INTERFACE,
    USB_KEYBOARD_INTERFACE_INDEX,
    0x00U, // Value used to select this alternate setting for the interface identified in the prior field
    USB_KEYBOARD_ENDPOINT_COUNT,
    USB_KEYBOARD_CLASS,
    USB_KEYBOARD_SUBCLASS,
    USB_KEYBOARD_PROTOCOL,
    USB_STRING_DESCRIPTOR_ID_KEYBOARD,

// Keyboard HID descriptor

    USB_DESCRIPTOR_LENGTH_HID,
    USB_DESCRIPTOR_TYPE_HID,
    0x00U, 0x01U, // HID Class Specification release
    0x00U,        // Country code of the localized hardware
    0x01U,        // Number of class descriptors (at least one report descriptor)
    USB_DESCRIPTOR_TYPE_HID_REPORT,
    USB_SHORT_GET_LOW(USB_DESCRIPTOR_LENGTH_KEYBOARD_REPORT),
    USB_SHORT_GET_HIGH(USB_DESCRIPTOR_LENGTH_KEYBOARD_REPORT),

// Keyboard endpoint descriptor

    USB_DESCRIPTOR_LENGTH_ENDPOINT,
    USB_DESCRIPTOR_TYPE_ENDPOINT,
    USB_KEYBOARD_ENDPOINT_IN | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    // The address of the endpoint on the USB device described by this descriptor.
    USB_ENDPOINT_INTERRUPT, // This field describes the endpoint's attributes
    USB_SHORT_GET_LOW(USB_KEYBOARD_INTERRUPT_IN_PACKET_SIZE),
    USB_SHORT_GET_HIGH(USB_KEYBOARD_INTERRUPT_IN_PACKET_SIZE),
    // Maximum packet size this endpoint is capable of sending or receiving when this configuration is selected.
    USB_KEYBOARD_INTERRUPT_IN_POLL_INTERVAL,
};

uint8_t g_UsbDeviceString0[USB_DESCRIPTOR_LENGTH_STRING0] = {
    sizeof(g_UsbDeviceString0), USB_DESCRIPTOR_TYPE_STRING, 0x09U, 0x04U,
};

uint8_t g_UsbDeviceString1[USB_DESCRIPTOR_LENGTH_STRING1] = {
    sizeof(g_UsbDeviceString1),
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

uint8_t g_UsbDeviceString2[USB_DESCRIPTOR_LENGTH_STRING2] = {
    sizeof(g_UsbDeviceString2),
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

uint32_t g_UsbDeviceStringDescriptorLength[USB_DEVICE_STRING_COUNT] = {
    sizeof(g_UsbDeviceString0), sizeof(g_UsbDeviceString1), sizeof(g_UsbDeviceString2),
    sizeof(UsbMouseString), sizeof(UsbKeyboardString),
};

uint8_t *g_UsbDeviceStringDescriptorArray[USB_DEVICE_STRING_COUNT] = {
    g_UsbDeviceString0, g_UsbDeviceString1, g_UsbDeviceString2, UsbMouseString, UsbKeyboardString,
};

usb_language_t g_UsbDeviceLanguage[USB_DEVICE_LANGUAGE_COUNT] = {{
    g_UsbDeviceStringDescriptorArray, g_UsbDeviceStringDescriptorLength, (uint16_t)0x0409U,
}};

usb_language_list_t g_UsbDeviceLanguageList = {
    g_UsbDeviceString0, sizeof(g_UsbDeviceString0), g_UsbDeviceLanguage, USB_DEVICE_LANGUAGE_COUNT,
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
    if (USB_COMPOSITE_CONFIGURE_INDEX > configurationDescriptor->configuration) {
        configurationDescriptor->buffer = UsbConfigurationDescriptor;
        configurationDescriptor->length = USB_DESCRIPTOR_LENGTH_CONFIGURATION_ALL;
        return kStatus_USB_Success;
    }
    return kStatus_USB_InvalidRequest;
}

usb_status_t USB_DeviceGetStringDescriptor(
    usb_device_handle handle, usb_device_get_string_descriptor_struct_t *stringDescriptor)
{
    if (stringDescriptor->stringIndex == 0U) {
        stringDescriptor->buffer = (uint8_t *)g_UsbDeviceLanguageList.languageString;
        stringDescriptor->length = g_UsbDeviceLanguageList.stringLength;
    } else {
        uint8_t languageId = 0U;
        uint8_t languageIndex = USB_DEVICE_STRING_COUNT;

        for (; languageId < USB_DEVICE_STRING_COUNT; languageId++) {
            if (stringDescriptor->languageId == g_UsbDeviceLanguageList.languageList[languageId].languageId) {
                if (stringDescriptor->stringIndex < USB_DEVICE_STRING_COUNT) {
                    languageIndex = stringDescriptor->stringIndex;
                }
                break;
            }
        }

        if (USB_DEVICE_STRING_COUNT == languageIndex) {
            return kStatus_USB_InvalidRequest;
        }
        stringDescriptor->buffer = (uint8_t *)g_UsbDeviceLanguageList.languageList[languageId].string[languageIndex];
        stringDescriptor->length = g_UsbDeviceLanguageList.languageList[languageId].length[languageIndex];
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
        hidReportDescriptor->buffer = UsbDeviceMouseReportDescriptor;
        hidReportDescriptor->length = USB_DESCRIPTOR_LENGTH_MOUSE_REPORT;
    } else if (USB_KEYBOARD_INTERFACE_INDEX == hidReportDescriptor->interfaceNumber) {
        hidReportDescriptor->buffer = UsbKeyboardReportDescriptor;
        hidReportDescriptor->length = USB_DESCRIPTOR_LENGTH_KEYBOARD_REPORT;
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
