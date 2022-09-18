#include "usb_api.h"
#include "usb_descriptor_hid.h"
#include "usb_interfaces/usb_interface_generic_hid.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_interfaces/usb_interface_system_keyboard.h"
#include "usb_interfaces/usb_interface_mouse.h"
#include "usb_descriptor_mouse_report.h"
#include "usb_descriptor_generic_hid_report.h"

usb_status_t USB_DeviceGetHidReportDescriptor(
    usb_device_handle handle, usb_device_get_hid_report_descriptor_struct_t *hidReportDescriptor)
{
    switch (hidReportDescriptor->interfaceNumber) {
        case USB_GENERIC_HID_INTERFACE_INDEX:
            hidReportDescriptor->buffer = (uint8_t*)UsbGenericHidReportDescriptor;
            hidReportDescriptor->length = USB_GENERIC_HID_REPORT_DESCRIPTOR_LENGTH;
            break;
        case USB_BASIC_KEYBOARD_INTERFACE_INDEX:
            hidReportDescriptor->buffer = (uint8_t*)UsbBasicKeyboardReportDescriptor;
            hidReportDescriptor->length = USB_BASIC_KEYBOARD_REPORT_DESCRIPTOR_LENGTH;
            break;
        case USB_MEDIA_KEYBOARD_INTERFACE_INDEX:
            hidReportDescriptor->buffer = (uint8_t*)UsbMediaKeyboardReportDescriptor;
            hidReportDescriptor->length = USB_MEDIA_KEYBOARD_REPORT_DESCRIPTOR_LENGTH;
            break;
        case USB_SYSTEM_KEYBOARD_INTERFACE_INDEX:
            hidReportDescriptor->buffer = (uint8_t*)UsbSystemKeyboardReportDescriptor;
            hidReportDescriptor->length = USB_SYSTEM_KEYBOARD_REPORT_DESCRIPTOR_LENGTH;
            break;
        case USB_MOUSE_INTERFACE_INDEX:
            hidReportDescriptor->buffer = (uint8_t*)UsbMouseReportDescriptor;
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
