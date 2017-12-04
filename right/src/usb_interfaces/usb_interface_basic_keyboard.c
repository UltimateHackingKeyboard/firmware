#include "usb_composite_device.h"

usb_device_class_struct_t UsbBasicKeyboardClass = {
    .type = kUSB_DeviceClassTypeHid,
    .configurations = USB_DEVICE_CONFIGURATION_COUNT,
    .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
        .count = USB_BASIC_KEYBOARD_INTERFACE_COUNT,
        .interfaces = (usb_device_interfaces_struct_t[USB_BASIC_KEYBOARD_INTERFACE_COUNT]) {{
            .classCode = USB_CLASS_HID,
            .subclassCode = USB_HID_SUBCLASS_BOOT,
            .protocolCode = USB_HID_PROTOCOL_KEYBOARD,
            .interfaceNumber = USB_BASIC_KEYBOARD_INTERFACE_INDEX,
            .count = 1,
            .interface = (usb_device_interface_struct_t[]) {{
                .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
                .classSpecific = NULL,
                .endpointList = {
                    USB_BASIC_KEYBOARD_ENDPOINT_COUNT, (usb_device_endpoint_struct_t[USB_BASIC_KEYBOARD_ENDPOINT_COUNT]) {{
                        .endpointAddress = USB_BASIC_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
                        .transferType = USB_ENDPOINT_INTERRUPT,
                        .maxPacketSize = USB_BASIC_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
                    }}
                }
            }}
        }}
    }}
};

static usb_basic_keyboard_report_t usbBasicKeyboardReports[2];
uint32_t UsbBasicKeyboardActionCounter;
usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport = usbBasicKeyboardReports;
bool IsUsbBasicKeyboardReportSent = false;

usb_basic_keyboard_report_t* getInactiveUsbBasicKeyboardReport(void)
{
    return ActiveUsbBasicKeyboardReport == usbBasicKeyboardReports ? usbBasicKeyboardReports+1 : usbBasicKeyboardReports;
}

void SwitchActiveUsbBasicKeyboardReport(void)
{
    ActiveUsbBasicKeyboardReport = getInactiveUsbBasicKeyboardReport();
}

void ResetActiveUsbBasicKeyboardReport(void)
{
    bzero(ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
}

static usb_status_t UsbBasicKeyboardAction(void)
{
    usb_status_t status = USB_DeviceHidSend(
        UsbCompositeDevice.basicKeyboardHandle, USB_BASIC_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t*)getInactiveUsbBasicKeyboardReport(), USB_BASIC_KEYBOARD_REPORT_LENGTH);
    IsUsbBasicKeyboardReportSent = true;
    UsbBasicKeyboardActionCounter++;
    return status;
}

usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbBasicKeyboardAction();
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

usb_status_t UsbBasicKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbBasicKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbBasicKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_BASIC_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbBasicKeyboardAction();
    }
    return kStatus_USB_Error;
}
