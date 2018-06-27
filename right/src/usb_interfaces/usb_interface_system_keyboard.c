#include "usb_composite_device.h"

uint32_t UsbSystemKeyboardActionCounter;
static usb_system_keyboard_report_t usbSystemKeyboardReports[2];
usb_system_keyboard_report_t* ActiveUsbSystemKeyboardReport = usbSystemKeyboardReports;
volatile bool IsUsbSystemKeyboardReportSent = false;

static usb_system_keyboard_report_t* getInactiveUsbSystemKeyboardReport()
{
    return ActiveUsbSystemKeyboardReport == usbSystemKeyboardReports ? usbSystemKeyboardReports+1 : usbSystemKeyboardReports;
}

void SwitchActiveUsbSystemKeyboardReport(void)
{
    ActiveUsbSystemKeyboardReport = getInactiveUsbSystemKeyboardReport();
}

void ResetActiveUsbSystemKeyboardReport(void)
{
    bzero(ActiveUsbSystemKeyboardReport, USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
}

static usb_status_t UsbSystemKeyboardAction(void)
{
    usb_status_t status = USB_DeviceHidSend(
        UsbCompositeDevice.systemKeyboardHandle, USB_SYSTEM_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t*)getInactiveUsbSystemKeyboardReport(), USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
    IsUsbSystemKeyboardReportSent = true;
    UsbSystemKeyboardActionCounter++;
    return status;
}

usb_status_t UsbSystemKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbSystemKeyboardAction();
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

usb_status_t UsbSystemKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbSystemKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbSystemKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_SYSTEM_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbSystemKeyboardAction();
    }
    return kStatus_USB_Error;
}
