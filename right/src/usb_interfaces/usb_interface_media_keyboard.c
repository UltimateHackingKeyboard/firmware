#include "usb_composite_device.h"
#include "usb_report_updater.h"

uint32_t UsbMediaKeyboardActionCounter;
static usb_media_keyboard_report_t usbMediaKeyboardReports[2];
usb_media_keyboard_report_t* ActiveUsbMediaKeyboardReport = usbMediaKeyboardReports;

static usb_media_keyboard_report_t* GetInactiveUsbMediaKeyboardReport(void)
{
    return ActiveUsbMediaKeyboardReport == usbMediaKeyboardReports ? usbMediaKeyboardReports+1 : usbMediaKeyboardReports;
}

static void SwitchActiveUsbMediaKeyboardReport(void)
{
    ActiveUsbMediaKeyboardReport = GetInactiveUsbMediaKeyboardReport();
}

void UsbMediaKeyboardResetActiveReport(void)
{
    bzero(ActiveUsbMediaKeyboardReport, USB_MEDIA_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbMediaKeyboardAction(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.mediaKeyboardHandle, USB_MEDIA_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbMediaKeyboardReport, USB_MEDIA_KEYBOARD_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbMediaKeyboardActionCounter++;
        SwitchActiveUsbMediaKeyboardReport();
    }
    return usb_status;
}

usb_status_t UsbMediaKeyboardCheckIdleElapsed()
{
    return kStatus_USB_Busy;
}

usb_status_t UsbMediaKeyboardCheckReportReady()
{
    if (memcmp(ActiveUsbMediaKeyboardReport, GetInactiveUsbMediaKeyboardReport(), sizeof(usb_media_keyboard_report_t)) != 0)
        return kStatus_USB_Success;

    return UsbMediaKeyboardCheckIdleElapsed();
}

usb_status_t UsbMediaKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
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
            UsbReportUpdateSemaphore &= ~(1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX);
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
            break;

        case kUSB_DeviceHidEventGetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportId == 0 && report->reportLength <= USB_MEDIA_KEYBOARD_REPORT_LENGTH) {
                report->reportBuffer = (void*)ActiveUsbMediaKeyboardReport;
                UsbMediaKeyboardActionCounter++;
                SwitchActiveUsbMediaKeyboardReport();
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        default:
            break;
    }

    return error;
}
