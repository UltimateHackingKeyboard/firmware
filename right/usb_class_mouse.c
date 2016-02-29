#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "usb_descriptor_device.h"
#include "composite.h"
#include "usb_class_mouse.h"

static usb_device_endpoint_struct_t UsbMouseEndpoints[USB_MOUSE_ENDPOINT_COUNT] = {{
    USB_MOUSE_ENDPOINT_ID | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_MOUSE_INTERRUPT_IN_PACKET_SIZE,
}};

static usb_device_interface_struct_t UsbMouseInterface[] = {{
    USB_MOUSE_INTERFACE_ALTERNATE_SETTING,
    {USB_MOUSE_ENDPOINT_COUNT, UsbMouseEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbMouseInterfaces[USB_MOUSE_INTERFACE_COUNT] = {{
    USB_MOUSE_CLASS,
    USB_MOUSE_SUBCLASS,
    USB_MOUSE_PROTOCOL,
    USB_MOUSE_INTERFACE_INDEX,
    UsbMouseInterface,
    sizeof(UsbMouseInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbMouseInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_MOUSE_INTERFACE_COUNT,
    UsbMouseInterfaces,
}};

usb_device_class_struct_t UsbMouseClass = {
    UsbMouseInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};
