#include "usb_composite_device.h"
#include "usb_report_updater.h"

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
    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.mouseHandle, USB_MOUSE_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbMouseReport, USB_MOUSE_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbMouseActionCounter++;
        SwitchActiveUsbMouseReport();
    }
    return usb_status;
}

usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Success;

    switch (event) {
        // This report is received when the report has been sent
        case kUSB_DeviceHidEventSendResponse:
            UsbReportUpdateSemaphore &= ~(1 << USB_MOUSE_INTERFACE_INDEX);
            break;
        case kUSB_DeviceHidEventRecvResponse:
        case kUSB_DeviceHidEventGetReport:
        case kUSB_DeviceHidEventSetReport:
        case kUSB_DeviceHidEventRequestReportBuffer:
        case kUSB_DeviceHidEventGetIdle:
        case kUSB_DeviceHidEventGetProtocol:
        case kUSB_DeviceHidEventSetIdle:
        case kUSB_DeviceHidEventSetProtocol:
        default:
            break;
    }
    return error;
}

usb_status_t UsbMouseSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    return kStatus_USB_Success;
}

usb_status_t UsbMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    return kStatus_USB_Success;
}
