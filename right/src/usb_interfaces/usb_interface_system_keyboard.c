#include "usb_composite_device.h"
#include "usb_report_updater.h"
#include "timer.h"

uint32_t UsbSystemKeyboardActionCounter;
static usb_system_keyboard_report_t usbSystemKeyboardReports[2];
usb_system_keyboard_report_t* ActiveUsbSystemKeyboardReport = usbSystemKeyboardReports;
static uint32_t usbSystemKeyboardReportLastSendTime = 0;

static usb_system_keyboard_report_t* GetInactiveUsbSystemKeyboardReport()
{
    return ActiveUsbSystemKeyboardReport == usbSystemKeyboardReports ? usbSystemKeyboardReports+1 : usbSystemKeyboardReports;
}

static void SwitchActiveUsbSystemKeyboardReport(void)
{
    ActiveUsbSystemKeyboardReport = GetInactiveUsbSystemKeyboardReport();
}

void UsbSystemKeyboardResetActiveReport(void)
{
    bzero(ActiveUsbSystemKeyboardReport, USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbSystemKeyboardAction(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.systemKeyboardHandle, USB_SYSTEM_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbSystemKeyboardReport, USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbSystemKeyboardActionCounter++;
        SwitchActiveUsbSystemKeyboardReport();
    }
    return usb_status;
}

usb_status_t UsbSystemKeyboardCheckIdleElapsed()
{
    uint16_t idlePeriodUs = ((usb_device_hid_struct_t*)UsbCompositeDevice.systemKeyboardHandle)->idleRate * 4 * 1000; // idleRate is in 4ms units.
    if (!idlePeriodUs) {
        return kStatus_USB_Busy;
    }

    bool hasIdleElapsed = Timer_GetElapsedTimeMicros(&usbSystemKeyboardReportLastSendTime) > idlePeriodUs;
    return hasIdleElapsed ? kStatus_USB_Success : kStatus_USB_Busy;
}

usb_status_t UsbSystemKeyboardCheckReportReady()
{
    if (memcmp(ActiveUsbSystemKeyboardReport, GetInactiveUsbSystemKeyboardReport(), sizeof(usb_system_keyboard_report_t)) != 0)
        return kStatus_USB_Success;

    return UsbSystemKeyboardCheckIdleElapsed();
}

usb_status_t UsbSystemKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            UsbReportUpdateSemaphore &= ~(1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX);
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceHidEventRecvResponse:
            error = kStatus_USB_InvalidRequest;
            break;

        case kUSB_DeviceHidEventGetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportId == 0 && report->reportLength <= USB_SYSTEM_KEYBOARD_REPORT_LENGTH) {
                report->reportBuffer = (void*)ActiveUsbSystemKeyboardReport;
                UsbSystemKeyboardActionCounter++;
                SwitchActiveUsbSystemKeyboardReport();
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        // SetReport is not required for this interface.
        case kUSB_DeviceHidEventSetReport:
        case kUSB_DeviceHidEventRequestReportBuffer:
            error = kStatus_USB_InvalidRequest;
            break;

        case kUSB_DeviceHidEventGetIdle:
            error = kStatus_USB_Success;
            break;

        case kUSB_DeviceHidEventSetIdle:
            usbSystemKeyboardReportLastSendTime = CurrentTime;
            error = kStatus_USB_Success;
            break;

        // No boot protocol support for this interface.
        case kUSB_DeviceHidEventGetProtocol:
        case kUSB_DeviceHidEventSetProtocol:
            error = kStatus_USB_InvalidRequest;
            break;

        default:
            break;
    }

    return error;
}

usb_status_t UsbSystemKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    return kStatus_USB_Error;
}

usb_status_t UsbSystemKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    return kStatus_USB_Error;
}
