#include "usb_composite_device.h"
#include "usb_protocol_handler.h"
#include "timer.h"

uint32_t UsbGenericHidActionCounter;
uint8_t GenericHidOutBuffer[USB_GENERIC_HID_OUT_BUFFER_LENGTH];
uint8_t GenericHidInBuffer[USB_GENERIC_HID_IN_BUFFER_LENGTH];
static uint32_t usbGenericHidReportLastSendTime = 0;

static usb_status_t UsbReceiveData(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

    return USB_DeviceHidRecv(UsbCompositeDevice.genericHidHandle,
                             USB_GENERIC_HID_ENDPOINT_OUT_INDEX,
                             GenericHidOutBuffer,
                             USB_GENERIC_HID_OUT_BUFFER_LENGTH);
}

usb_status_t UsbGenericHidCheckIdleElapsed()
{
    uint16_t idlePeriodMs = ((usb_device_hid_struct_t*)UsbCompositeDevice.genericHidHandle)->idleRate * 4; // idleRate is in 4ms units.
    if (!idlePeriodMs) {
        return kStatus_USB_Busy;
    }

    bool hasIdleElapsed = (Timer_GetElapsedTimeMicros(&usbGenericHidReportLastSendTime) / 1000) > idlePeriodMs;
    return hasIdleElapsed ? kStatus_USB_Success : kStatus_USB_Busy;
}

usb_status_t UsbGenericHidCheckReportReady()
{
    return UsbGenericHidCheckIdleElapsed();
}

usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_InvalidRequest;

    switch (event) {
        case ((uint32_t)-kUSB_DeviceEventSetConfiguration):
            if (*(uint8_t*)param > 0) {
                error = UsbReceiveData();
            }
            break;
        case ((uint32_t)-kUSB_DeviceEventSetInterface):
            if (*(uint8_t*)param == 0) {
                error = UsbReceiveData();
            }
            break;

        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
            break;

        case kUSB_DeviceHidEventRecvResponse:
            UsbProtocolHandler();

            USB_DeviceHidSend(UsbCompositeDevice.genericHidHandle,
                              USB_GENERIC_HID_ENDPOINT_IN_INDEX,
                              GenericHidInBuffer,
                              USB_GENERIC_HID_IN_BUFFER_LENGTH);
            UsbGenericHidActionCounter++;
            error = UsbReceiveData();
            break;

        case kUSB_DeviceHidEventGetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportId == 0 && report->reportLength <= USB_GENERIC_HID_IN_BUFFER_LENGTH) {
                report->reportBuffer = GenericHidInBuffer;
                UsbGenericHidActionCounter++;
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventSetIdle:
            usbGenericHidReportLastSendTime = CurrentTime;
            error = kStatus_USB_Success;
            break;

        default:
            break;
    }

    return error;
}
