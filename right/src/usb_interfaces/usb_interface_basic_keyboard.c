#include "led_display.h"
#include "usb_composite_device.h"
#include "usb_report_updater.h"
#include "timer.h"

static usb_basic_keyboard_report_t usbBasicKeyboardReports[2];
static uint8_t usbBasicKeyboardInBuffer[USB_BASIC_KEYBOARD_REPORT_LENGTH];
static uint32_t usbBasicKeyboardReportLastSendTime = 0;
uint32_t UsbBasicKeyboardActionCounter;
usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport = usbBasicKeyboardReports;

static usb_basic_keyboard_report_t* GetInactiveUsbBasicKeyboardReport(void)
{
    return ActiveUsbBasicKeyboardReport == usbBasicKeyboardReports ? usbBasicKeyboardReports+1 : usbBasicKeyboardReports;
}

static void SwitchActiveUsbBasicKeyboardReport(void)
{
    ActiveUsbBasicKeyboardReport = GetInactiveUsbBasicKeyboardReport();
}

void UsbBasicKeyboardResetActiveReport(void)
{
    bzero(ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbBasicKeyboardAction(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.basicKeyboardHandle, USB_BASIC_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        usbBasicKeyboardReportLastSendTime = CurrentTime;
        UsbBasicKeyboardActionCounter++;
        SwitchActiveUsbBasicKeyboardReport();
    }
    return usb_status;
}

usb_status_t UsbBasicKeyboardCheckIdleElapsed()
{
    uint16_t idlePeriodMs = ((usb_device_hid_struct_t*)UsbCompositeDevice.basicKeyboardHandle)->idleRate * 4; // idleRate is in 4ms units.
    if (!idlePeriodMs) {
        return kStatus_USB_Busy;
    }

    bool hasIdleElapsed = (Timer_GetElapsedTimeMicros(&usbBasicKeyboardReportLastSendTime) / 1000) > idlePeriodMs;
    return hasIdleElapsed ? kStatus_USB_Success : kStatus_USB_Busy;
}

usb_status_t UsbBasicKeyboardCheckReportReady()
{
    if (memcmp(ActiveUsbBasicKeyboardReport, GetInactiveUsbBasicKeyboardReport(), sizeof(usb_basic_keyboard_report_t)) != 0)
        return kStatus_USB_Success;

    return UsbBasicKeyboardCheckIdleElapsed();
}

usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
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
            UsbReportUpdateSemaphore &= ~(1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX);
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
            break;

        case kUSB_DeviceHidEventGetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportId == 0 && report->reportLength <= USB_BASIC_KEYBOARD_REPORT_LENGTH) {
                report->reportBuffer = (void*)ActiveUsbBasicKeyboardReport;

                usbBasicKeyboardReportLastSendTime = CurrentTime;
                UsbBasicKeyboardActionCounter++;
                SwitchActiveUsbBasicKeyboardReport();
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventSetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_OUPUT && report->reportId == 0 && report->reportLength == 1) {
                LedDisplay_SetIcon(LedDisplayIcon_CapsLock, report->reportBuffer[0] & HID_KEYBOARD_LED_CAPSLOCK);
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }
        case kUSB_DeviceHidEventRequestReportBuffer: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportLength <= USB_BASIC_KEYBOARD_REPORT_LENGTH) {
                report->reportBuffer = usbBasicKeyboardInBuffer;
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_AllocFail;
            }
            break;
        }

        case kUSB_DeviceHidEventSetIdle:
            usbBasicKeyboardReportLastSendTime = CurrentTime;
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
