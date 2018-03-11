#include "led_display.h"
#include "usb_composite_device.h"

static usb_basic_keyboard_report_t usbBasicKeyboardReports[2];
uint32_t UsbBasicKeyboardActionCounter;
usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport = usbBasicKeyboardReports;
volatile bool IsUsbBasicKeyboardReportSent = false;
static uint8_t usbBasicKeyboardInBuffer[USB_BASIC_KEYBOARD_REPORT_LENGTH];

usb_basic_keyboard_report_t* getInactiveUsbBasicKeyboardReport(void)
{
    return ActiveUsbBasicKeyboardReport == usbBasicKeyboardReports ? usbBasicKeyboardReports+1 : usbBasicKeyboardReports;
}

void SwitchActiveUsbBasicKeyboardReport(void)
{
    ActiveUsbBasicKeyboardReport = getInactiveUsbBasicKeyboardReport();
}

void ResetActiveUsbBasicKeyboardReport(void)
{
    bzero(ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
}

static usb_status_t UsbBasicKeyboardAction(void)
{
    usb_status_t status = USB_DeviceHidSend(
        UsbCompositeDevice.basicKeyboardHandle, USB_BASIC_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t*)getInactiveUsbBasicKeyboardReport(), USB_BASIC_KEYBOARD_REPORT_LENGTH);
    IsUsbBasicKeyboardReportSent = true;
    UsbBasicKeyboardActionCounter++;
    return status;
}

usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbBasicKeyboardAction();
            }
            break;
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
                error = kStatus_USB_InvalidRequest;
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
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbBasicKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbBasicKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_BASIC_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbBasicKeyboardAction();
    }
    return kStatus_USB_Error;
}
