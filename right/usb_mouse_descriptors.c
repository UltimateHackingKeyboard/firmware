#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "hid_mouse.h"
#include "hid_keyboard.h"

static usb_device_endpoint_struct_t g_UsbDeviceHidMouseEndpoints[USB_MOUSE_ENDPOINT_COUNT] =
{
    {
        USB_MOUSE_ENDPOINT_IN | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_INTERRUPT,
        USB_MOUSE_INTERRUPT_IN_PACKET_SIZE,
    },
};

static usb_device_interface_struct_t g_UsbDeviceHidMouseInterface[] =
{
    {
        USB_MOUSE_INTERFACE_ALTERNATE_SETTING,
        {
            USB_MOUSE_ENDPOINT_COUNT,
            g_UsbDeviceHidMouseEndpoints,
        },
        NULL,
    }
};

static usb_device_interfaces_struct_t g_UsbDeviceHidMouseInterfaces[USB_MOUSE_INTERFACE_COUNT] =
{
    {
        USB_MOUSE_CLASS,
        USB_MOUSE_SUBCLASS,
        USB_MOUSE_PROTOCOL,
        USB_MOUSE_INTERFACE_INDEX,
        g_UsbDeviceHidMouseInterface,
        sizeof(g_UsbDeviceHidMouseInterface) / sizeof(usb_device_interfaces_struct_t),
    },
};

static usb_device_interface_list_t g_UsbDeviceHidMouseInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] =
{
    {
        USB_MOUSE_INTERFACE_COUNT,
        g_UsbDeviceHidMouseInterfaces,
    },
};

usb_device_class_struct_t UsbDeviceMouseConfig = {
    g_UsbDeviceHidMouseInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

uint8_t UsbDeviceMouseReportDescriptor[USB_DESCRIPTOR_LENGTH_MOUSE_REPORT] = {
    0x05U, 0x01U, // Usage Page (Generic Desktop)
    0x09U, 0x02U, // Usage (Mouse)
    0xA1U, 0x01U, // Collection (Application)
    0x09U, 0x01U, // Usage (Pointer)

    0xA1U, 0x00U, // Collection (Physical)
    0x05U, 0x09U, // Usage Page (Buttons)
    0x19U, 0x01U, // Usage Minimum (01U)
    0x29U, 0x03U, // Usage Maximum (03U)

    0x15U, 0x00U, // Logical Minimum (0U)
    0x25U, 0x01U, // Logical Maximum (1U)
    0x95U, 0x03U, // Report Count (3U)
    0x75U, 0x01U, // Report Size (1U)

    0x81U, 0x02U, // Input(Data, Variable, Absolute) 3U button bit fields
    0x95U, 0x01U, // Report count (1U)
    0x75U, 0x05U, // Report Size (5U)
    0x81U, 0x01U, // Input (Constant), 5U constant field

    0x05U, 0x01U, // Usage Page (Generic Desktop)
    0x09U, 0x30U, // Usage (X)
    0x09U, 0x31U, // Usage (Y)
    0x09U, 0x38U, // Usage (Z)

    0x15U, 0x81U, // Logical Minimum (-127)
    0x25U, 0x7FU, // Logical Maximum (127)
    0x75U, 0x08U, // Report Size (8U)
    0x95U, 0x03U, // Report Count (3U)

    0x81U, 0x06U, // Input(Data, Variable, Relative), 3 position bytes (X & Y & Z)
    0xC0U,        // End collection, Close Pointer collection
    0xC0U         // End collection, Close Mouse collection
};

uint8_t UsbMouseString[USB_MOUSE_STRING_DESCRIPTOR_LENGTH] = {
    sizeof(UsbMouseString),
    USB_DESCRIPTOR_TYPE_STRING,
    'H', 0x00U,
    'I', 0x00U,
    'D', 0x00U,
    ' ', 0x00U,
    'M', 0x00U,
    'O', 0x00U,
    'U', 0x00U,
    'S', 0x00U,
    'E', 0x00U,
    ' ', 0x00U,
    'D', 0x00U,
    'E', 0x00U,
    'V', 0x00U,
    'I', 0x00U,
    'C', 0x00U,
    'E', 0x00U,
};
