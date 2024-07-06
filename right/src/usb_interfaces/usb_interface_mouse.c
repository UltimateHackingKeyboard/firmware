#include "usb_composite_device.h"
#include "usb_report_updater.h"

static usb_mouse_report_t usbMouseReports[2];
static uint8_t usbMouseFeatBuffer[USB_MOUSE_FEAT_REPORT_LENGTH];
usb_hid_protocol_t usbMouseProtocol;
uint32_t UsbMouseActionCounter;
usb_mouse_report_t* ActiveUsbMouseReport = usbMouseReports;

bool UsbMouse_HighResMode = false;

static usb_mouse_report_t* GetInactiveUsbMouseReport(void)
{
    return ActiveUsbMouseReport == usbMouseReports ? usbMouseReports+1 : usbMouseReports;
}

static void SwitchActiveUsbMouseReport(void)
{
    ActiveUsbMouseReport = GetInactiveUsbMouseReport();
}

void UsbMouseResetActiveReport(void)
{
    bzero(ActiveUsbMouseReport, USB_MOUSE_REPORT_LENGTH);
}

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
                usbMouseFeatBuffer[0] = (uint8_t)UsbMouse_HighResMode;
                usbMouseFeatBuffer[1] = (uint8_t)UsbMouse_HighResMode;
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
                // according to the windows [enhanced wheel support in
                // windows](https://learn.microsoft.com/en-us/previous-versions/windows/hardware/design/dn613912(v=vs.85))
                // whitepaper, the setreport request *should* include all
                // values (as we'd expect for a proper request). so re-write
                // this to either (TODO):
                // 1. succeed iff the given values match the actual (logical)
                //    max; or
                // 2. read the values and set appropriate variables, and use
                //    those variables when putting together the mouse report.
                // windows' own whitepaper says that they only ever use the
                // physical max, and linux follows suit, so there are no known
                // implementations that actually set the value. keeping the
                // logical min/max set to 0/1 so that the value is effectively
                // binary should make this behaviour consistent even for some
                // unknown host that implements this properly. however it *is*
                // possible via windows registry keys to disable high-res
                // scrolling for a single axis only, so it might be worth
                // reading these values.
                // NB: there will need to be exceptions for linux, which sends a
                // broken report: it sends only one value instead of two (or
                // zero values if only one multiplier is defined in the
                // descriptor)
                // * when the length is zero, this case isn't invoked after the
                //   kUSB_DeviceHidEventRequestReportBuffer case, so that one
                //   must also return success in order for the kernel to use the
                //   physical max
                // NB: high-res scrolling should be used iff this request
                // returns success *and* the values in the setreport request
                // indicate that it should be used.
                // * if we return a failure, linux uses the *minimum* physical
                //   value, i.e., disables high-res scrolling
                // * TODO: verify that windows does the same (and other windows
                //   behaviour)
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventRequestReportBuffer: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_FEATURE && report->reportId == 0 && report->reportLength <= sizeof(usbMouseFeatBuffer)) {
                // see notes above
                UsbMouse_HighResMode = true;
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
