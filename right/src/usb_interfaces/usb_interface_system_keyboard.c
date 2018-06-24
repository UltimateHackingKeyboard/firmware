#include "usb_composite_device.h"

uint32_t UsbSystemKeyboardActionCounter;
static usb_system_keyboard_report_t usbSystemKeyboardReports[2];
usb_system_keyboard_report_t* ActiveUsbSystemKeyboardReport = usbSystemKeyboardReports;

usb_system_keyboard_report_t* GetInactiveUsbSystemKeyboardReport()
{
    return ActiveUsbSystemKeyboardReport == usbSystemKeyboardReports ? usbSystemKeyboardReports+1 : usbSystemKeyboardReports;
}

static void SwitchActiveUsbSystemKeyboardReport(void)
{
    ActiveUsbSystemKeyboardReport = GetInactiveUsbSystemKeyboardReport();
}

void ResetActiveUsbSystemKeyboardReport(void)
{
    bzero(ActiveUsbSystemKeyboardReport, USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbSystemKeyboardAction(void)
{
    if (((usb_device_hid_struct_t *)UsbCompositeDevice.systemKeyboardHandle)->interruptInPipeBusy)
        return kStatus_USB_Busy; // The previous report has not been sent yet

    UsbSystemKeyboardActionCounter++;
    SwitchActiveUsbSystemKeyboardReport(); // Switch the active report
    usb_status_t usb_status = USB_DeviceHidSend(
            UsbCompositeDevice.systemKeyboardHandle, USB_SYSTEM_KEYBOARD_ENDPOINT_INDEX,
            (uint8_t*)GetInactiveUsbSystemKeyboardReport(), USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
    if (usb_status != kStatus_USB_Success) {
        SwitchActiveUsbSystemKeyboardReport(); // Switch back, as the command failed
    }
    return usb_status;
}

usb_status_t UsbSystemKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        // This report is received when the report has been sent
        case kUSB_DeviceHidEventSendResponse:
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceHidEventRecvResponse:
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
        //return UsbSystemKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbSystemKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_SYSTEM_KEYBOARD_INTERFACE_INDEX == interface) {
        //return UsbSystemKeyboardAction();
    }
    return kStatus_USB_Error;
}
