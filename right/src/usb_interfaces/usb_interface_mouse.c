#include "usb_composite_device.h"

uint32_t UsbMouseActionCounter;
static usb_mouse_report_t usbMouseReports[2];
usb_mouse_report_t* ActiveUsbMouseReport = usbMouseReports;

usb_mouse_report_t* GetInactiveUsbMouseReport(void)
{
    return ActiveUsbMouseReport == usbMouseReports ? usbMouseReports+1 : usbMouseReports;
}

static void SwitchActiveUsbMouseReport(void)
{
    ActiveUsbMouseReport = GetInactiveUsbMouseReport();
}

void ResetActiveUsbMouseReport(void)
{
    bzero(ActiveUsbMouseReport, USB_MOUSE_REPORT_LENGTH);
}

usb_status_t usbMouseAction(void)
{
    if (!UsbCompositeDevice.attach)
        return kStatus_USB_Error; // The device is not attached

    if (((usb_device_hid_struct_t *)UsbCompositeDevice.mouseHandle)->interruptInPipeBusy)
        return kStatus_USB_Busy; // The previous report has not been sent yet

    UsbMouseActionCounter++;
    SwitchActiveUsbMouseReport(); // Switch the active report
    usb_status_t usb_status = USB_DeviceHidSend(
            UsbCompositeDevice.mouseHandle, USB_MOUSE_ENDPOINT_INDEX,
            (uint8_t*)GetInactiveUsbMouseReport(), USB_MOUSE_REPORT_LENGTH);
    if (usb_status != kStatus_USB_Success) {
        SwitchActiveUsbMouseReport(); // Switch back, as the command failed
    }
    return usb_status;
}

usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        // This report is received when the report has been sent
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
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

usb_status_t UsbMouseSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    return kStatus_USB_Error;
}

usb_status_t UsbMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    return kStatus_USB_Error;
}
