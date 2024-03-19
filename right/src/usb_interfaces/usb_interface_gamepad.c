#include "usb_composite_device.h"
#include "usb_report_updater.h"

static usb_gamepad_report_t usbGamepadReports[2];
uint32_t UsbGamepadActionCounter;
usb_gamepad_report_t* ActiveUsbGamepadReport = usbGamepadReports;

#ifndef __ZEPHYR__
/* key: usb_gamepad_property_t, value: shift for button in X360 buttons field */
static const uint8_t x360padButtonMap[] = {
    12, 13, 14, 15, // ABXY
    8, 9,           // bumpers
    0xFF, 0xFF,     // triggers are analog
    5, 4,           // BACK, START
    6, 7,           // sticks
    0, 1, 2, 3,     // D-pad
};

static usb_gamepad_report_t* GetInactiveUsbGamepadReport(void)
{
    return ActiveUsbGamepadReport == usbGamepadReports ? usbGamepadReports+1 : usbGamepadReports;
}

static void SwitchActiveUsbGamepadReport(void)
{
    ActiveUsbGamepadReport = GetInactiveUsbGamepadReport();
}

void UsbGamepadResetActiveReport(void)
{
    bzero(ActiveUsbGamepadReport, USB_GAMEPAD_REPORT_LENGTH);
    ActiveUsbGamepadReport->X360.reportSize = USB_GAMEPAD_REPORT_LENGTH;
}

usb_status_t UsbGamepadAction(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.gamepadHandle, USB_GAMEPAD_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbGamepadReport, USB_GAMEPAD_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbGamepadActionCounter++;
        SwitchActiveUsbGamepadReport();
    }

    return usb_status;
}

usb_status_t UsbGamepadCheckIdleElapsed()
{
    return kStatus_USB_Busy;
}

usb_status_t UsbGamepadCheckReportReady()
{
    if (memcmp(ActiveUsbGamepadReport, GetInactiveUsbGamepadReport(), sizeof(usb_gamepad_report_t)) != 0)
        return kStatus_USB_Success;

    return UsbGamepadCheckIdleElapsed();
}

void UsbGamepadSetProperty(usb_gamepad_report_t* report, usb_gamepad_property_t key, int value)
{
    if ((key <= _GAMEPAD_BUTTONS_END) /* && (key >= _GAMEPAD_BUTTONS_BEGIN)*/) {
        uint16_t flag = 1 << x360padButtonMap[key];
        if (!value) {
            report->X360.buttons &= ~flag;
        } else {
            report->X360.buttons |= flag;
        }
    } else if ((key == GAMEPAD_LEFT_TRIGGER_ANALOG) || (key == GAMEPAD_RIGHT_TRIGGER_ANALOG)) {
        *(&report->X360.lTrigger + (key - GAMEPAD_LEFT_TRIGGER_ANALOG)) = value;
    } else {
        *(&report->X360.lX + (key - GAMEPAD_LEFT_STICK_X)) = value;
    }
}

usb_status_t UsbGamepadCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_InvalidRequest;

    switch (event) {
        case ((uint32_t)-kUSB_DeviceEventSetConfiguration):
            usbGamepadReports[0].X360.reportSize = USB_GAMEPAD_REPORT_LENGTH;
            usbGamepadReports[1].X360.reportSize = USB_GAMEPAD_REPORT_LENGTH;
            error = kStatus_USB_Success;
            break;
        case ((uint32_t)-kUSB_DeviceEventSetInterface):
            if (*(uint8_t*)param == 0) {
                error = kStatus_USB_Success;
            }
            break;

        case kUSB_DeviceHidEventSendResponse:
            UsbReportUpdateSemaphore &= ~(1 << USB_GAMEPAD_INTERFACE_INDEX);
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
            break;

        case kUSB_DeviceHidEventGetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportId == 0 && report->reportLength <= USB_GAMEPAD_REPORT_LENGTH) {
                report->reportBuffer = (void*)ActiveUsbGamepadReport;
                UsbGamepadActionCounter++;
                SwitchActiveUsbGamepadReport();
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
#endif
