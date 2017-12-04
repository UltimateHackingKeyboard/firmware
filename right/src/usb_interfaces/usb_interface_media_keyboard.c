#include "usb_composite_device.h"

usb_device_class_struct_t UsbMediaKeyboardClass = {
    .type = kUSB_DeviceClassTypeHid,
    .configurations = USB_DEVICE_CONFIGURATION_COUNT,
    .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
        .count = USB_MEDIA_KEYBOARD_INTERFACE_COUNT,
        .interfaces = (usb_device_interfaces_struct_t[USB_MEDIA_KEYBOARD_INTERFACE_COUNT]) {{
            .classCode = USB_CLASS_HID,
            .subclassCode = USB_HID_SUBCLASS_BOOT,
            .protocolCode = USB_HID_PROTOCOL_KEYBOARD,
            .interfaceNumber = USB_MEDIA_KEYBOARD_INTERFACE_INDEX,
            .count = 1,
            .interface = (usb_device_interface_struct_t[]) {{
                .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
                .classSpecific = NULL,
                .endpointList = {
                    USB_MEDIA_KEYBOARD_ENDPOINT_COUNT,
                    (usb_device_endpoint_struct_t[USB_MEDIA_KEYBOARD_ENDPOINT_COUNT]) {{
                        .endpointAddress = USB_MEDIA_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
                        .transferType = USB_ENDPOINT_INTERRUPT,
                        .maxPacketSize = USB_MEDIA_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
                    }}
                }
            }}
        }}
    }}
};

uint32_t UsbMediaKeyboardActionCounter;
static usb_media_keyboard_report_t usbMediaKeyboardReports[2];
usb_media_keyboard_report_t* ActiveUsbMediaKeyboardReport = usbMediaKeyboardReports;
bool IsUsbMediaKeyboardReportSent = false;

usb_media_keyboard_report_t* getInactiveUsbMediaKeyboardReport(void)
{
    return ActiveUsbMediaKeyboardReport == usbMediaKeyboardReports ? usbMediaKeyboardReports+1 : usbMediaKeyboardReports;
}

void SwitchActiveUsbMediaKeyboardReport(void)
{
    ActiveUsbMediaKeyboardReport = getInactiveUsbMediaKeyboardReport();
}

void ResetActiveUsbMediaKeyboardReport(void)
{
    bzero(ActiveUsbMediaKeyboardReport, USB_MEDIA_KEYBOARD_REPORT_LENGTH);
}

static usb_status_t UsbMediaKeyboardAction(void)
{
    usb_status_t status = USB_DeviceHidSend(
        UsbCompositeDevice.mediaKeyboardHandle, USB_MEDIA_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t*)getInactiveUsbMediaKeyboardReport(), USB_MEDIA_KEYBOARD_REPORT_LENGTH);
    IsUsbMediaKeyboardReportSent = true;
    UsbMediaKeyboardActionCounter++;
    return status;
}

usb_status_t UsbMediaKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbMediaKeyboardAction();
            }
            break;
        case kUSB_DeviceHidEventGetReport:
        case kUSB_DeviceHidEventSetReport:
        case kUSB_DeviceHidEventRequestReportBuffer:
            error = kStatus_USB_InvalidRequest;
            break;
        case kUSB_DeviceHidEventGetIdle:
        case kUSB_DeviceHidEventGetProtocol:
        case kUSB_DeviceHidEventSetIdle:
        case kUSB_DeviceHidEventSetProtocol:
            break;
        default:
            break;
    }

    return error;
}

usb_status_t UsbMediaKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbMediaKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbMediaKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_MEDIA_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbMediaKeyboardAction();
    }
    return kStatus_USB_Error;
}
