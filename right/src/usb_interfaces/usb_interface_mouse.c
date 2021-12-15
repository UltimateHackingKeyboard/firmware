#include "usb_composite_device.h"
#include "usb_report_updater.h"
#include "timer.h"

static usb_mouse_report_t usbMouseReports[2];
static uint32_t usbMouseReportLastSendTime = 0;
uint32_t UsbMouseActionCounter;
usb_mouse_report_t* ActiveUsbMouseReport = usbMouseReports;

static usb_mouse_report_t* GetInactiveUsbMouseReport(void)
{
    return ActiveUsbMouseReport == usbMouseReports ? usbMouseReports+1 : usbMouseReports;
}

static void SwitchActiveUsbMouseReport(void)
{
    ActiveUsbMouseReport = GetInactiveUsbMouseReport();
}

void UsbMouseResetActiveReport(void)
{
    bzero(ActiveUsbMouseReport, USB_MOUSE_REPORT_LENGTH);
}

usb_status_t UsbMouseAction(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.mouseHandle, USB_MOUSE_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbMouseReport, USB_MOUSE_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbMouseActionCounter++;
        SwitchActiveUsbMouseReport();
    }
    return usb_status;
}

usb_status_t UsbMouseCheckIdleElapsed()
{
    uint16_t idlePeriodMs = ((usb_device_hid_struct_t*)UsbCompositeDevice.mouseHandle)->idleRate * 4; // idleRate is in 4ms units.
    if (!idlePeriodMs) {
        return kStatus_USB_Busy;
    }

    bool hasIdleElapsed = (Timer_GetElapsedTimeMicros(&usbMouseReportLastSendTime) / 1000) > idlePeriodMs;
    return hasIdleElapsed ? kStatus_USB_Success : kStatus_USB_Busy;
}

usb_status_t UsbMouseCheckReportReady()
{
    if (memcmp(ActiveUsbMouseReport, GetInactiveUsbMouseReport(), sizeof(usb_mouse_report_t)) != 0)
        return kStatus_USB_Success;

    return UsbMouseCheckIdleElapsed();
}

usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_device_hid_struct_t *hidHandle = (usb_device_hid_struct_t *)handle;
    usb_status_t error = kStatus_USB_InvalidRequest;

    switch (event) {
        case ((uint32_t)-kUSB_DeviceEventSetConfiguration):
            error = kStatus_USB_Success;
            break;
        case ((uint32_t)-kUSB_DeviceEventSetInterface):
            if (*(uint8_t*)param == 0) {
                error = kStatus_USB_Success;
            }
            break;

        case kUSB_DeviceHidEventSendResponse:
            UsbReportUpdateSemaphore &= ~(1 << USB_MOUSE_INTERFACE_INDEX);
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
            break;

        case kUSB_DeviceHidEventGetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportId == 0 && report->reportLength <= USB_MOUSE_REPORT_LENGTH) {
                report->reportBuffer = (void*)ActiveUsbMouseReport;
                UsbMouseActionCounter++;
                SwitchActiveUsbMouseReport();
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventSetIdle:
            usbMouseReportLastSendTime = CurrentTime;
            error = kStatus_USB_Success;
            break;

        case kUSB_DeviceHidEventSetProtocol: {
            uint8_t report = *(uint16_t*)param;
            if (report <= 1) {
                hidHandle->protocol = report;
                error = kStatus_USB_Success;
            }
            else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        default:
            break;
    }

    return error;
}
