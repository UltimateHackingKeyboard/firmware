#include <string.h>
#include "usb_composite_device.h"
#include "usb_report_updater.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

uint32_t UsbSystemKeyboardActionCounter;
static usb_system_keyboard_report_t usbSystemKeyboardReports[2];
usb_system_keyboard_report_t* ActiveUsbSystemKeyboardReport = usbSystemKeyboardReports;

static usb_system_keyboard_report_t* GetInactiveUsbSystemKeyboardReport()
{
    return ActiveUsbSystemKeyboardReport == usbSystemKeyboardReports ? usbSystemKeyboardReports+1 : usbSystemKeyboardReports;
}

void UsbSystemKeyboardResetActiveReport(void)
{
    memset(ActiveUsbSystemKeyboardReport, 0, USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
}

#ifndef __ZEPHYR__

static void SwitchActiveUsbSystemKeyboardReport(void)
{
    ActiveUsbSystemKeyboardReport = GetInactiveUsbSystemKeyboardReport();
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


usb_status_t UsbSystemKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
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
            UsbReportUpdateSemaphore &= ~(1 << USB_SYSTEM_KEYBOARD_INTERFACE_INDEX);
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
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

        default:
            break;
    }

    return error;
}
#endif

usb_status_t UsbSystemKeyboardCheckIdleElapsed()
{
    return kStatus_USB_Busy;
}

usb_status_t UsbSystemKeyboardCheckReportReady()
{
    if (memcmp(ActiveUsbSystemKeyboardReport, GetInactiveUsbSystemKeyboardReport(), sizeof(usb_system_keyboard_report_t)) != 0)
        return kStatus_USB_Success;

    return UsbSystemKeyboardCheckIdleElapsed();
}

bool UsbSystemKeyboard_AddScancode(usb_system_keyboard_report_t* report, uint8_t scancode)
{
    if (!UsbSystemKeyboard_UsedScancode(scancode))
        return false;

    set_bit(scancode - USB_SYSTEM_KEYBOARD_MIN_BITFIELD_SCANCODE, report->bitfield);
    return true;
}

void UsbSystemKeyboard_RemoveScancode(usb_system_keyboard_report_t* report, uint8_t scancode)
{
    if (!UsbSystemKeyboard_UsedScancode(scancode))
        return;

    clear_bit(scancode - USB_SYSTEM_KEYBOARD_MIN_BITFIELD_SCANCODE, report->bitfield);
}

void UsbSystemKeyboard_MergeReports(const usb_system_keyboard_report_t* sourceReport, usb_system_keyboard_report_t* targetReport)
{
    for (uint8_t i = 0; i < ARRAY_SIZE(targetReport->bitfield); i++) {
        targetReport->bitfield[i] |= sourceReport->bitfield[i];
    }
}
