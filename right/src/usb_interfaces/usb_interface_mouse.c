#include "usb_composite_device.h"
#include "usb_report_updater.h"
#include <string.h>
#include "event_scheduler.h"

#ifdef __ZEPHYR__
#include "usb/usb_compatibility.h"
#endif


#include "usb_descriptors/usb_descriptor_mouse_report.h"

static usb_mouse_report_t usbMouseReports[2];
static uint8_t usbMouseFeatBuffer[USB_MOUSE_FEAT_REPORT_LENGTH];
usb_hid_protocol_t usbMouseProtocol;
uint32_t UsbMouseActionCounter;
usb_mouse_report_t* ActiveUsbMouseReport = usbMouseReports;

int16_t UsbMouseScrollMultiplier = USB_MOUSE_REPORT_DESCRIPTOR_MIN_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE;

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

usb_hid_protocol_t UsbMouseGetProtocol(void)
{
    return usbMouseProtocol;
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

    // latch the active protocol to avoid ISR <-> Thread race
    usbMouseProtocol = ((usb_device_hid_struct_t*)UsbCompositeDevice.mouseHandle)->protocol;

    return usb_status;
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
            if (report->reportId != 0) {
                error = kStatus_USB_InvalidRequest;
            } else if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportLength <= USB_MOUSE_REPORT_LENGTH) {
                report->reportBuffer = (void*)ActiveUsbMouseReport;
                UsbMouseActionCounter++;
                SwitchActiveUsbMouseReport();
                error = kStatus_USB_Success;
            } else if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_FEATURE) {
                usbMouseFeatBuffer[0] = (uint8_t)(UsbMouseScrollMultiplier != USB_MOUSE_REPORT_DESCRIPTOR_MIN_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE);
                report->reportBuffer = usbMouseFeatBuffer;
                report->reportLength = sizeof(usbMouseFeatBuffer);
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventSetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_FEATURE && report->reportId == 0 && report->reportLength <= sizeof(usbMouseFeatBuffer)) {
                // With a single resolution multiplier, this case will never be
                // hit on Linux (for multiple resolution multipliers, one value
                // will be missing, so would have to be inferred from the
                // other(s)). But Windows does use this request properly, so it
                // needs to be handled appropriately.
                UsbMouseScrollMultiplier = usbMouseFeatBuffer[0] ? USB_MOUSE_REPORT_DESCRIPTOR_MAX_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE : USB_MOUSE_REPORT_DESCRIPTOR_MIN_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE;
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventRequestReportBuffer: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_FEATURE && report->reportId == 0 && report->reportLength <= sizeof(usbMouseFeatBuffer)) {
                // The Linux implementation of SetReport when initializing a
                // device with a single resolution multiplier value is broken,
                // sending an empty report, and as a result the
                // kUSB_DeviceHidEventSetReport case above isn't triggered at
                // all; but it only sends this report when it detects the
                // resolution multiplier, and the intention is to activate the
                // feature, so turn high-res mode on here.
                UsbMouseScrollMultiplier = USB_MOUSE_REPORT_DESCRIPTOR_MAX_RESOLUTION_MULTIPLIER_PHYSICAL_VALUE;
                report->reportBuffer = usbMouseFeatBuffer;
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_AllocFail;
            }
            break;
        }

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
#endif


void UsbMouseSendActiveReport(void)
{
#ifdef __ZEPHYR__
    UsbCompatibility_SendMouseReport(ActiveUsbMouseReport);
    SwitchActiveUsbMouseReport();
#else
    UsbReportUpdateSemaphore |= 1 << USB_MOUSE_INTERFACE_INDEX;
    usb_status_t status = UsbMouseAction();
    if (status != kStatus_USB_Success) {
        UsbReportUpdateSemaphore &= ~(1 << USB_MOUSE_INTERFACE_INDEX);
        EventVector_Set(EventVector_SendUsbReports);
    }
#endif
}

usb_status_t UsbMouseCheckIdleElapsed()
{
    return kStatus_USB_Busy;
}

usb_status_t UsbMouseCheckReportReady(bool* buttonsChanged)
{
    // Send out the mouse position and wheel values continuously if the report is not zeros, but only send the mouse button states when they change.
    if ((memcmp(ActiveUsbMouseReport, GetInactiveUsbMouseReport(), sizeof(usb_mouse_report_t)) != 0) ||
            ActiveUsbMouseReport->x || ActiveUsbMouseReport->y ||
            ActiveUsbMouseReport->wheelX || ActiveUsbMouseReport->wheelY) {
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

