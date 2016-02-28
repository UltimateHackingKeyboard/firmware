#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "usb_device_descriptor.h"
#include "usb_generic_hid_descriptors.h"

static usb_device_endpoint_struct_t UsbGenericHidEndpoints[USB_GENERIC_HID_ENDPOINT_COUNT] =
{
    {
        USB_GENERIC_HID_ENDPOINT_IN_ID | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_INTERRUPT,
        USB_GENERIC_HID_INTERRUPT_IN_PACKET_SIZE,
    },
    {
        USB_GENERIC_HID_ENDPOINT_OUT_ID | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_INTERRUPT,
        USB_GENERIC_HID_INTERRUPT_OUT_PACKET_SIZE,
    }
};

static usb_device_interface_struct_t UsbGenericHidInterface[] = {{
    USB_KEYBOARD_INTERFACE_ALTERNATE_SETTING,
    {USB_GENERIC_HID_ENDPOINT_COUNT, UsbGenericHidEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbGenericHidInterfaces[USB_GENERIC_HID_INTERFACE_COUNT] = {{
    USB_GENERIC_HID_CLASS,
    USB_GENERIC_HID_SUBCLASS,
    USB_GENERIC_HID_PROTOCOL,
    USB_GENERIC_HID_INTERFACE_INDEX,
    UsbGenericHidInterface,
    sizeof(UsbGenericHidInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbGenericHidInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_GENERIC_HID_INTERFACE_COUNT,
    UsbGenericHidInterfaces,
}};

usb_device_class_struct_t UsbGenericHidClass = {
    UsbGenericHidInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

uint8_t UsbGenericHidReportDescriptor[USB_GENERIC_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05U, 0x81U, /* Usage Page (Vendor defined)*/
    0x09U, 0x82U, /* Usage (Vendor defined) */
    0xA1U, 0x01U, /* Collection (Application) */
    0x09U, 0x83U, /* Usage (Vendor defined) */

    0x09U, 0x84U, /* Usage (Vendor defined) */
    0x15U, 0x80U, /* Logical Minimum (-128) */
    0x25U, 0x7FU, /* Logical Maximum (127) */
    0x75U, 0x08U, /* Report Size (8U) */
    0x95U, 0x08U, /* Report Count (8U) */
    0x81U, 0x02U, /* Input(Data, Variable, Absolute) */

    0x09U, 0x84U, /* Usage (Vendor defined) */
    0x15U, 0x80U, /* logical Minimum (-128) */
    0x25U, 0x7FU, /* logical Maximum (127) */
    0x75U, 0x08U, /* Report Size (8U) */
    0x95U, 0x08U, /* Report Count (8U) */
    0x91U, 0x02U, /* Input (Data, Variable, Absolute) */
    0xC0U,        /* End collection */
};

uint8_t UsbGenericHidString[USB_GENERIC_HID_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbGenericHidString),
    USB_DESCRIPTOR_TYPE_STRING,
    'H', 0x00U,
    'I', 0x00U,
    'D', 0x00U,
    ' ', 0x00U,
    'G', 0x00U,
    'E', 0x00U,
    'N', 0x00U,
    'E', 0x00U,
    'R', 0x00U,
    'I', 0x00U,
    'C', 0x00U,
    ' ', 0x00U,
    'D', 0x00U,
    'E', 0x00U,
    'V', 0x00U,
    'I', 0x00U,
    'C', 0x00U,
    'E', 0x00U,
};
