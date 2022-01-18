#include "led_display.h"
#include "usb_composite_device.h"
#include "usb_report_updater.h"

static usb_basic_keyboard_report_t usbBasicKeyboardReports[2];
static uint8_t usbBasicKeyboardOutBuffer[USB_BASIC_KEYBOARD_OUT_REPORT_LENGTH];
usb_hid_protocol_t usbBasicKeyboardProtocol;
uint32_t UsbBasicKeyboardActionCounter;
usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport = usbBasicKeyboardReports;

static usb_basic_keyboard_report_t* GetInactiveUsbBasicKeyboardReport(void)
{
    return ActiveUsbBasicKeyboardReport == usbBasicKeyboardReports ? usbBasicKeyboardReports+1 : usbBasicKeyboardReports;
}

static void SwitchActiveUsbBasicKeyboardReport(void)
{
    ActiveUsbBasicKeyboardReport = GetInactiveUsbBasicKeyboardReport();
}

void UsbBasicKeyboardResetActiveReport(void)
{
    bzero(ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
}

usb_hid_protocol_t UsbBasicKeyboardGetProtocol(void)
{
    return USB_HID_BOOT_PROTOCOL;
}

usb_status_t UsbBasicKeyboardAction(void)
{
    if (!UsbCompositeDevice.attach) {
        return kStatus_USB_Error; // The device is not attached
    }

    usb_status_t usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.basicKeyboardHandle, USB_BASIC_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbBasicKeyboardActionCounter++;
        SwitchActiveUsbBasicKeyboardReport();
    }

    // latch the active protocol to avoid ISR <-> Thread race
    usbBasicKeyboardProtocol = ((usb_device_hid_struct_t*)UsbCompositeDevice.basicKeyboardHandle)->protocol;

    return usb_status;
}

usb_status_t UsbBasicKeyboardCheckIdleElapsed()
{
    return kStatus_USB_Busy;
}

usb_status_t UsbBasicKeyboardCheckReportReady()
{
    if (memcmp(ActiveUsbBasicKeyboardReport, GetInactiveUsbBasicKeyboardReport(), sizeof(usb_basic_keyboard_report_t)) != 0)
        return kStatus_USB_Success;

    return UsbBasicKeyboardCheckIdleElapsed();
}

usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
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
            UsbReportUpdateSemaphore &= ~(1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX);
            if (UsbCompositeDevice.attach) {
                error = kStatus_USB_Success;
            }
            break;

        case kUSB_DeviceHidEventGetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_INPUT && report->reportId == 0 && report->reportLength <= USB_BASIC_KEYBOARD_REPORT_LENGTH) {
                report->reportBuffer = (void*)ActiveUsbBasicKeyboardReport;

                UsbBasicKeyboardActionCounter++;
                SwitchActiveUsbBasicKeyboardReport();
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }

        case kUSB_DeviceHidEventSetReport: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportType == USB_DEVICE_HID_REQUEST_GET_REPORT_TYPE_OUPUT && report->reportId == 0 && report->reportLength == sizeof(usbBasicKeyboardOutBuffer)) {
                LedDisplay_SetIcon(LedDisplayIcon_CapsLock, report->reportBuffer[0] & HID_KEYBOARD_LED_CAPSLOCK);
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        }
        case kUSB_DeviceHidEventRequestReportBuffer: {
            usb_device_hid_report_struct_t *report = (usb_device_hid_report_struct_t*)param;
            if (report->reportLength <= sizeof(usbBasicKeyboardOutBuffer)) {
                report->reportBuffer = usbBasicKeyboardOutBuffer;
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

static void setRolloverError(usb_basic_keyboard_report_t* report)
{
    if (report->scancodes[0] != HID_KEYBOARD_SC_ERROR_ROLLOVER) {
        memset(report->scancodes, HID_KEYBOARD_SC_ERROR_ROLLOVER, ARRAY_SIZE(report->scancodes));
    }
}

void UsbBasicKeyboard_AddScancode(usb_basic_keyboard_report_t* report, uint8_t scancode, uint8_t* idx)
{
    if (scancode == 0)
        return;

    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        if (idx == NULL) {
            for (uint8_t i = 0; i < ARRAY_SIZE(report->scancodes); i++) {
                if (report->scancodes[i] == 0) {
                    report->scancodes[i] = scancode;
                    return;
                }
            }
        } else if (*idx < ARRAY_SIZE(report->scancodes)) {
            report->scancodes[(*idx)++] = scancode;
            return;
        } else {
            /* invalid index */
        }

        setRolloverError(report);
    }
}

void UsbBasicKeyboard_RemoveScancode(usb_basic_keyboard_report_t* report, uint8_t scancode)
{
    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        for (uint8_t i = 0; i < ARRAY_SIZE(report->scancodes); i++) {
            if (report->scancodes[i] == scancode) {
                report->scancodes[i] = 0;
                return;
            }
        }
    }
}

bool UsbBasicKeyboard_ContainsScancode(const usb_basic_keyboard_report_t* report, uint8_t scancode)
{
    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        for (uint8_t i = 0; i < ARRAY_SIZE(report->scancodes); i++) {
            if (report->scancodes[i] == scancode) {
                return true;
            }
        }
        return false;
    } else {
        return false;
    }
}

size_t UsbBasicKeyboard_ScancodeCount(const usb_basic_keyboard_report_t* report)
{
    size_t size = 0;
    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        while ((size < ARRAY_SIZE(report->scancodes)) && (report->scancodes[size] != 0)) {
            size++;
        }
    }
    return size;
}

void UsbBasicKeyboard_MergeReports(const usb_basic_keyboard_report_t* sourceReport, usb_basic_keyboard_report_t* targetReport, uint8_t* idx)
{
    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        targetReport->modifiers |= sourceReport->modifiers;
        for (uint8_t i = 0; (i < ARRAY_SIZE(sourceReport->scancodes)) && (sourceReport->scancodes[i] != 0); i++) {
            if (*idx < ARRAY_SIZE(sourceReport->scancodes)) {
                targetReport->scancodes[(*idx)++] = sourceReport->scancodes[i];
            } else {
                setRolloverError(targetReport);
                return;
            }
        }
    }
}

void UsbBasicKeyboard_ForeachScancode(const usb_basic_keyboard_report_t* report, void(*action)(uint8_t))
{
    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        for (uint8_t i = 0; (i < ARRAY_SIZE(report->scancodes)) && (report->scancodes[i] != 0); i++) {
            action(report->scancodes[i]);
        }
    }
}
