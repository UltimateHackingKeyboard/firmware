#include "usb_composite_device.h"
#include "usb_protocol_handler.h"

uint32_t UsbGenericHidActionCounter;
uint8_t GenericHidIn[USB_GENERIC_HID_IN_BUFFER_LENGTH];
uint8_t GenericHidOut[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

static usb_status_t UsbReceiveData(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

    return USB_DeviceHidRecv(UsbCompositeDevice.genericHidHandle,
                             USB_GENERIC_HID_ENDPOINT_OUT_INDEX,
                             GenericHidOut,
                             USB_GENERIC_HID_OUT_BUFFER_LENGTH);
}

usb_status_t UsbGenericHidCheckIdleElapsed()
{
    return kStatus_USB_Busy;
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
            UsbProtocolHandler(GenericHidOut, GenericHidIn);

            USB_DeviceHidSend(UsbCompositeDevice.genericHidHandle,
                              USB_GENERIC_HID_ENDPOINT_IN_INDEX,
                              GenericHidIn,
                              USB_GENERIC_HID_IN_BUFFER_LENGTH);
            UsbGenericHidActionCounter++;
            error = UsbReceiveData();
            break;

        case kUSB_DeviceHidEventGetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportId == 0 && report->reportLength <= USB_GENERIC_HID_IN_BUFFER_LENGTH) {
                report->reportBuffer = GenericHidIn;
                UsbGenericHidActionCounter++;
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
