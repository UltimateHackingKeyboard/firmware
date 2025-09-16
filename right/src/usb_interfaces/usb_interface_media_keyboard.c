#include "usb_composite_device.h"
#include "usb_report_updater.h"
#include <string.h>
#include "utils.h"
#include "event_scheduler.h"


uint32_t UsbMediaKeyboardActionCounter;
static usb_media_keyboard_report_t usbMediaKeyboardReports[2];
usb_media_keyboard_report_t* ActiveUsbMediaKeyboardReport = usbMediaKeyboardReports;

static bool needsResending = false;

static usb_media_keyboard_report_t* GetInactiveUsbMediaKeyboardReport(void)
{
    return ActiveUsbMediaKeyboardReport == usbMediaKeyboardReports ? usbMediaKeyboardReports+1 : usbMediaKeyboardReports;
}

void UsbMediaKeyboardResetActiveReport(void)
{
    memset(ActiveUsbMediaKeyboardReport, 0, USB_MEDIA_KEYBOARD_REPORT_LENGTH);
}

void SwitchActiveUsbMediaKeyboardReport(void)
{
    ActiveUsbMediaKeyboardReport = GetInactiveUsbMediaKeyboardReport();
}

#ifndef __ZEPHYR__
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

void UsbMediaKeyboardSendActiveReport(void) {
    UsbReportUpdateSemaphore |= 1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX;
    usb_status_t status = UsbMediaKeyboardAction();
    if (status != kStatus_USB_Success) {
        UsbReportUpdateSemaphore &= ~(1 << USB_MEDIA_KEYBOARD_INTERFACE_INDEX);
        EventVector_Set(EventVector_ResendUsbReports);
        needsResending = true;
    } else {
        needsResending = false;
    }
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
#endif

usb_status_t UsbMediaKeyboardCheckIdleElapsed()
{
    return kStatus_USB_Busy;
}

usb_status_t UsbMediaKeyboardCheckReportReady(bool resending)
{
    if (memcmp(ActiveUsbMediaKeyboardReport, GetInactiveUsbMediaKeyboardReport(), sizeof(usb_media_keyboard_report_t)) != 0 && (!resending || needsResending)) {
        return kStatus_USB_Success;
    }

    return UsbMediaKeyboardCheckIdleElapsed();
}


void UsbMediaKeyboard_MergeReports(const usb_media_keyboard_report_t* sourceReport, usb_media_keyboard_report_t* targetReport)
{
    uint8_t idx, i = 0;
    /* find empty position */
    for (idx = 0; idx < UTILS_ARRAY_SIZE(targetReport->scancodes); idx++) {
        if (targetReport->scancodes[idx] == 0) {
            break;
        }
    }
    /* copy into empty positions */
    while ((i < UTILS_ARRAY_SIZE(sourceReport->scancodes)) && (sourceReport->scancodes[i] != 0) && (idx < UTILS_ARRAY_SIZE(targetReport->scancodes))) {
        targetReport->scancodes[idx++] = sourceReport->scancodes[i++];
    }
}

bool UsbMediaKeyboard_AddScancode(usb_media_keyboard_report_t* report, uint16_t scancode)
{
    if (scancode == 0)
        return true;

    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->scancodes); i++) {
        if (report->scancodes[i] == 0) {
            report->scancodes[i] = scancode;
            return true;
        }
    }

    return false;
}

bool UsbMediaKeyboard_ContainsScancode(const usb_media_keyboard_report_t* report, uint16_t scancode)
{
    if (scancode == 0) {
        return false;
    }

    for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->scancodes); i++) {
        if (report->scancodes[i] == scancode) {
            return true;
        }
    }
    return false;
}
