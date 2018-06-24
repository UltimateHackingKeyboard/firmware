#include "usb_composite_device.h"

uint32_t UsbMediaKeyboardActionCounter;
static usb_media_keyboard_report_t usbMediaKeyboardReports[2];
usb_media_keyboard_report_t* ActiveUsbMediaKeyboardReport = usbMediaKeyboardReports;

usb_media_keyboard_report_t* GetInactiveUsbMediaKeyboardReport(void)
{
    return ActiveUsbMediaKeyboardReport == usbMediaKeyboardReports ? usbMediaKeyboardReports+1 : usbMediaKeyboardReports;
}

static void SwitchActiveUsbMediaKeyboardReport(void)
{
    ActiveUsbMediaKeyboardReport = GetInactiveUsbMediaKeyboardReport();
}

void ResetActiveUsbMediaKeyboardReport(void)
{
    bzero(ActiveUsbMediaKeyboardReport, USB_MEDIA_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbMediaKeyboardAction()
{
    if (((usb_device_hid_struct_t *)UsbCompositeDevice.mediaKeyboardHandle)->interruptInPipeBusy)
        return kStatus_USB_Busy; // The previous report has not been sent yet

    UsbMediaKeyboardActionCounter++;
    SwitchActiveUsbMediaKeyboardReport(); // Switch the active report
    usb_status_t usb_status = USB_DeviceHidSend(
            UsbCompositeDevice.mediaKeyboardHandle, USB_MEDIA_KEYBOARD_ENDPOINT_INDEX,
            (uint8_t*)GetInactiveUsbMediaKeyboardReport(), USB_MEDIA_KEYBOARD_REPORT_LENGTH);
    if (usb_status != kStatus_USB_Success) {
        SwitchActiveUsbMediaKeyboardReport(); // Switch back, as the command failed
    }
    return usb_status;
}

usb_status_t UsbMediaKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
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

usb_status_t UsbMediaKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    return kStatus_USB_Error;
}

usb_status_t UsbMediaKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    return kStatus_USB_Error;
}
