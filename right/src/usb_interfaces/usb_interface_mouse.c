#include "usb_composite_device.h"
#include "usb_report_updater.h"
#include <string.h>
#include "event_scheduler.h"
#include "attributes.h"
#include "macros/status_buffer.h"
#include "utils.h"

#ifdef __ZEPHYR__
#include "usb/usb_compatibility.h"
#endif

static bool needsResending = false;
static uint8_t retries = 0;

#include "usb_descriptors/usb_descriptor_mouse_report.h"

static usb_mouse_report_t usbMouseReports[2];

uint32_t UsbMouseActionCounter;
usb_mouse_report_t* ActiveUsbMouseReport = usbMouseReports;

static usb_mouse_report_t* GetInactiveUsbMouseReport(void)
{
    return ActiveUsbMouseReport == usbMouseReports ? usbMouseReports+1 : usbMouseReports;
}

void UsbMouseResetActiveReport(void)
{
    memset(ActiveUsbMouseReport, 0, USB_MOUSE_REPORT_LENGTH);
}

static void SwitchActiveUsbMouseReport(void)
{
    ActiveUsbMouseReport = GetInactiveUsbMouseReport();
}

#ifndef __ZEPHYR__

typedef struct {
#if USB_MOUSE_REPORT_ID
    uint8_t id;
#endif
    uint8_t scrollMultipliers;
} ATTR_PACKED usb_mouse_feature_report_t;

static usb_mouse_feature_report_t usbMouseFeatureReport = {
#if USB_MOUSE_REPORT_ID
    .id = USB_MOUSE_REPORT_ID
#endif
};

usb_status_t UsbMouseAction(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

#if USB_MOUSE_REPORT_ID
    ActiveUsbMouseReport->id = USB_MOUSE_REPORT_ID;
#endif
    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.mouseHandle, USB_MOUSE_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbMouseReport, USB_MOUSE_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbMouseActionCounter++;
        SwitchActiveUsbMouseReport();
    }

    return usb_status;
}

float VerticalScrollMultiplier(void)
{
    return usbMouseFeatureReport.scrollMultipliers & 0x01 ? USB_MOUSE_REPORT_DESCRIPTOR_MAX_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE : USB_MOUSE_REPORT_DESCRIPTOR_MIN_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE;
}

float HorizontalScrollMultiplier(void)
{
    return usbMouseFeatureReport.scrollMultipliers & 0x04 ? USB_MOUSE_REPORT_DESCRIPTOR_MAX_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE : USB_MOUSE_REPORT_DESCRIPTOR_MIN_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE;
}

usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_device_hid_struct_t *hidHandle = (usb_device_hid_struct_t *)handle;
    usb_status_t error = kStatus_USB_InvalidRequest;

    switch (event) {
        case ((uint32_t)-kUSB_DeviceEventSetConfiguration):
            usbMouseFeatureReport.scrollMultipliers = 0;
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
            if (report->reportId != USB_MOUSE_REPORT_ID) {
                error = kStatus_USB_InvalidRequest;
            } else if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT) {
                report->reportBuffer = (void*)ActiveUsbMouseReport;
                report->reportLength == USB_MOUSE_REPORT_LENGTH;
                UsbMouseActionCounter++;
                SwitchActiveUsbMouseReport();
                error = kStatus_USB_Success;
            } else if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_FEATURE) {
                report->reportBuffer = (void*)&usbMouseFeatureReport;
                report->reportLength = sizeof(usbMouseFeatureReport);
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventSetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_FEATURE &&
                report->reportLength == sizeof(usbMouseFeatureReport)) {
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventRequestReportBuffer: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_FEATURE) {
                report->reportBuffer = (void*)&usbMouseFeatureReport;
                report->reportLength = sizeof(usbMouseFeatureReport);
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_AllocFail;
            }
            break;
        }

        default:
            break;
    }

    return error;
}
#endif


void UsbMouseSendActiveReport(void)
{
#ifdef __ZEPHYR__
    UsbCompatibility_SendMouseReport(ActiveUsbMouseReport);
    SwitchActiveUsbMouseReport();
#else
    UsbReportUpdateSemaphore |= 1 << USB_MOUSE_INTERFACE_INDEX;
    usb_status_t status = UsbMouseAction();
    if (ShouldResendReport(status == kStatus_USB_Success, &retries)) {
        UsbReportUpdateSemaphore &= ~(1 << USB_MOUSE_INTERFACE_INDEX);
        EventVector_Set(EventVector_ResendUsbReports);
        needsResending = true;
    } else {
        needsResending = false;
    }
#endif
}

usb_status_t UsbMouseCheckIdleElapsed()
{
    return kStatus_USB_Busy;
}

usb_status_t UsbMouseCheckReportReady(bool resending, bool* buttonsChanged)
{
    // Send out the mouse position and wheel values continuously if the report is not zeros, but only send the mouse button states when they change.
    if (
            (!resending || needsResending) &&
            (
                (memcmp(ActiveUsbMouseReport, GetInactiveUsbMouseReport(), sizeof(usb_mouse_report_t)) != 0) ||
                ActiveUsbMouseReport->x || ActiveUsbMouseReport->y ||
                ActiveUsbMouseReport->wheelX || ActiveUsbMouseReport->wheelY
            )
       ) {
        if (buttonsChanged != NULL) {
            *buttonsChanged = ActiveUsbMouseReport->buttons != GetInactiveUsbMouseReport()->buttons;
        }
        return kStatus_USB_Success;
    }

    return UsbMouseCheckIdleElapsed();
}

void UsbMouse_MergeReports(usb_mouse_report_t* sourceReport, usb_mouse_report_t* targetReport)
{
    targetReport->buttons |= sourceReport->buttons;
    targetReport->x += sourceReport->x;
    targetReport->y += sourceReport->y;
    targetReport->wheelX += sourceReport->wheelX;
    targetReport->wheelY += sourceReport->wheelY;
    sourceReport->x = 0;
    sourceReport->y = 0;
    sourceReport->wheelX = 0;
    sourceReport->wheelY = 0;
}

