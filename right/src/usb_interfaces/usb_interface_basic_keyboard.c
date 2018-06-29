#include "led_display.h"
#include "usb_composite_device.h"

static usb_basic_keyboard_report_t usbBasicKeyboardReports[2];
uint32_t UsbBasicKeyboardActionCounter;
usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport = usbBasicKeyboardReports;
static uint8_t usbBasicKeyboardInBuffer[USB_BASIC_KEYBOARD_REPORT_LENGTH];

usb_basic_keyboard_report_t* GetInactiveUsbBasicKeyboardReport(void)
{
    return ActiveUsbBasicKeyboardReport == usbBasicKeyboardReports ? usbBasicKeyboardReports+1 : usbBasicKeyboardReports;
}

static void SwitchActiveUsbBasicKeyboardReport(void)
{
    ActiveUsbBasicKeyboardReport = GetInactiveUsbBasicKeyboardReport();
}

void ResetActiveUsbBasicKeyboardReport(void)
{
    bzero(ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbBasicKeyboardAction(void)
{
    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.basicKeyboardHandle, USB_BASIC_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbBasicKeyboardActionCounter++;
        SwitchActiveUsbBasicKeyboardReport();
    }
    return usb_status;
}

usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        // This report is received when the report has been sent
        case kUSB_DeviceHidEventSendResponse:
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceHidEventRecvResponse:
        case kUSB_DeviceHidEventGetReport:
            error = kStatus_USB_InvalidRequest;
            break;
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
    return kStatus_USB_Error;
}

usb_status_t UsbBasicKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    return kStatus_USB_Error;
}
