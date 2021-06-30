#include "usb_api.h"
#include "usb_descriptor_hid.h"
#include "usb_descriptor_mouse_report.h"
#include "usb_descriptor_generic_hid_report.h"
#include "usb_descriptor_configuration.h"

#define USB_GENERIC_HID_DESCRIPTOR_INDEX \
    (USB_DESCRIPTOR_LENGTH_CONFIGURE + USB_DESCRIPTOR_LENGTH_INTERFACE)

#define USB_BASIC_KEYBOARD_HID_DESCRIPTOR_INDEX \
    (USB_GENERIC_HID_DESCRIPTOR_INDEX + USB_DESCRIPTOR_LENGTH_HID + \
    2 * USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_INTERFACE)

#define USB_MEDIA_KEYBOARD_HID_DESCRIPTOR_INDEX \
    (USB_BASIC_KEYBOARD_HID_DESCRIPTOR_INDEX + USB_DESCRIPTOR_LENGTH_HID + \
    USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_INTERFACE)

#define USB_SYSTEM_KEYBOARD_HID_DESCRIPTOR_INDEX \
    (USB_MEDIA_KEYBOARD_HID_DESCRIPTOR_INDEX + USB_DESCRIPTOR_LENGTH_HID + \
    USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_INTERFACE)

#define USB_MOUSE_HID_DESCRIPTOR_INDEX \
    (USB_SYSTEM_KEYBOARD_HID_DESCRIPTOR_INDEX + USB_DESCRIPTOR_LENGTH_HID + \
    USB_DESCRIPTOR_LENGTH_ENDPOINT + USB_DESCRIPTOR_LENGTH_INTERFACE)

usb_status_t USB_DeviceGetHidDescriptor(
    usb_device_handle handle, usb_device_get_hid_descriptor_struct_t *hidDescriptor)
{
    hidDescriptor->length = USB_DESCRIPTOR_LENGTH_HID;

    switch (hidDescriptor->interfaceNumber) {
        case USB_GENERIC_HID_INTERFACE_INDEX:
            hidDescriptor->buffer = &UsbConfigurationDescriptor[USB_GENERIC_HID_DESCRIPTOR_INDEX];
            break;
        case USB_BASIC_KEYBOARD_INTERFACE_INDEX:
            hidDescriptor->buffer = &UsbConfigurationDescriptor[USB_BASIC_KEYBOARD_HID_DESCRIPTOR_INDEX];
            break;
        case USB_MEDIA_KEYBOARD_INTERFACE_INDEX:
            hidDescriptor->buffer = &UsbConfigurationDescriptor[USB_MEDIA_KEYBOARD_HID_DESCRIPTOR_INDEX];
            break;
        case USB_SYSTEM_KEYBOARD_INTERFACE_INDEX:
            hidDescriptor->buffer = &UsbConfigurationDescriptor[USB_SYSTEM_KEYBOARD_HID_DESCRIPTOR_INDEX];
            break;
        case USB_MOUSE_INTERFACE_INDEX:
            hidDescriptor->buffer = &UsbConfigurationDescriptor[USB_MOUSE_HID_DESCRIPTOR_INDEX];
            break;
        default:
            return kStatus_USB_InvalidRequest;
    }
    return kStatus_USB_Success;
}

usb_status_t USB_DeviceGetHidReportDescriptor(
    usb_device_handle handle, usb_device_get_hid_report_descriptor_struct_t *hidReportDescriptor)
{
    switch (hidReportDescriptor->interfaceNumber) {
        case USB_GENERIC_HID_INTERFACE_INDEX:
            hidReportDescriptor->buffer = UsbGenericHidReportDescriptor;
            hidReportDescriptor->length = USB_GENERIC_HID_REPORT_DESCRIPTOR_LENGTH;
            break;
        case USB_BASIC_KEYBOARD_INTERFACE_INDEX:
            hidReportDescriptor->buffer = UsbBasicKeyboardReportDescriptor;
            hidReportDescriptor->length = USB_BASIC_KEYBOARD_REPORT_DESCRIPTOR_LENGTH;
            break;
        case USB_MEDIA_KEYBOARD_INTERFACE_INDEX:
            hidReportDescriptor->buffer = UsbMediaKeyboardReportDescriptor;
            hidReportDescriptor->length = USB_MEDIA_KEYBOARD_REPORT_DESCRIPTOR_LENGTH;
            break;
        case USB_SYSTEM_KEYBOARD_INTERFACE_INDEX:
            hidReportDescriptor->buffer = UsbSystemKeyboardReportDescriptor;
            hidReportDescriptor->length = USB_SYSTEM_KEYBOARD_REPORT_DESCRIPTOR_LENGTH;
            break;
        case USB_MOUSE_INTERFACE_INDEX:
            hidReportDescriptor->buffer = UsbMouseReportDescriptor;
            hidReportDescriptor->length = USB_MOUSE_REPORT_DESCRIPTOR_LENGTH;
            break;
        default:
            return kStatus_USB_InvalidRequest;
    }
    return kStatus_USB_Success;
}

usb_status_t USB_DeviceGetHidPhysicalDescriptor(
    usb_device_handle handle, usb_device_get_hid_physical_descriptor_struct_t *hidPhysicalDescriptor)
{
    return kStatus_USB_InvalidRequest;
}
