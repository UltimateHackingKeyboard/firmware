#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "usb_descriptor_device.h"
#include "usb_class_generic_hid.h"

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
    USB_GENERIC_HID_INTERFACE_ALTERNATE_SETTING,
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
