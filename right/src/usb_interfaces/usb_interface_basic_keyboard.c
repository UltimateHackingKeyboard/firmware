#include <string.h>
#include "led_display.h"
#include "lufa/HIDClassCommon.h"
#include "macros/core.h"
#include "macro_events.h"
#include "usb_composite_device.h"
#include "usb_report_updater.h"

#ifdef __ZEPHYR__
#include "usb/usb_compatibility.h"
#endif

#include "utils.h"

#ifndef USB_HID_BOOT_PROTOCOL
#define USB_HID_BOOT_PROTOCOL   0U
#define USB_HID_REPORT_PROTOCOL   1U
#endif

static usb_basic_keyboard_report_t usbBasicKeyboardReports[2];
uint32_t UsbBasicKeyboardActionCounter;
usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport = usbBasicKeyboardReports;

bool UsbBasicKeyboard_CapsLockOn = false;
bool UsbBasicKeyboard_NumLockOn = false;
bool UsbBasicKeyboard_ScrollLockOn = false;

usb_hid_protocol_t usbBasicKeyboardProtocol;

static void setRolloverError(usb_basic_keyboard_report_t* report);

usb_hid_protocol_t UsbBasicKeyboardGetProtocol(void)
{
#ifdef __ZEPHYR__
    return USB_HID_REPORT_PROTOCOL;
#else
    return usbBasicKeyboardProtocol;
#endif
}


usb_basic_keyboard_report_t* GetInactiveUsbBasicKeyboardReport(void)
{
    return ActiveUsbBasicKeyboardReport == usbBasicKeyboardReports ? usbBasicKeyboardReports+1 : usbBasicKeyboardReports;
}

void UsbBasicKeyboardResetActiveReport(void)
{
    memset(ActiveUsbBasicKeyboardReport, 0, USB_BASIC_KEYBOARD_REPORT_LENGTH);
}

void SwitchActiveUsbBasicKeyboardReport(void)
{
    ActiveUsbBasicKeyboardReport = GetInactiveUsbBasicKeyboardReport();
}


#ifndef __ZEPHYR__

static void processStateChange(bool *targetVar, bool value, bool *onChange)
{
    if (value != *targetVar) {
        *targetVar = value;
        *onChange = true;
        EventVector_Set(EventVector_KeyboardLedState);
    }
}


void UsbBasicKeyboard_HandleProtocolChange()
{
    if (usbBasicKeyboardProtocol != ((usb_device_hid_struct_t*)UsbCompositeDevice.basicKeyboardHandle)->protocol) {
        // The protocol changed while the report was assembled
        UsbBasicKeyboardResetActiveReport();
        Macros_ResetBasicKeyboardReports();

        // latch the active protocol to avoid ISR <-> Thread race
        usbBasicKeyboardProtocol = ((usb_device_hid_struct_t*)UsbCompositeDevice.basicKeyboardHandle)->protocol;

        EventVector_Unset(EventVector_ProtocolChanged);
    }
}

static uint8_t usbBasicKeyboardOutBuffer[USB_BASIC_KEYBOARD_OUT_REPORT_LENGTH];

usb_status_t UsbBasicKeyboardAction(void)
{
    usb_status_t usb_status = kStatus_USB_Error;

    if (!UsbCompositeDevice.attach) {
        return usb_status; // The device is not attached
    }

    if (usbBasicKeyboardProtocol != ((usb_device_hid_struct_t*)UsbCompositeDevice.basicKeyboardHandle)->protocol) {
        UsbBasicKeyboard_HandleProtocolChange();
        return usb_status;
    }

    usb_status = USB_DeviceHidSend(
        UsbCompositeDevice.basicKeyboardHandle, USB_BASIC_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t *)ActiveUsbBasicKeyboardReport, UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL ?
                USB_BOOT_KEYBOARD_REPORT_LENGTH : USB_BASIC_KEYBOARD_REPORT_LENGTH);
    if (usb_status == kStatus_USB_Success) {
        UsbBasicKeyboardActionCounter++;
        SwitchActiveUsbBasicKeyboardReport();
    }

    return usb_status;
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

                processStateChange(&UsbBasicKeyboard_CapsLockOn,   report->reportBuffer[0] & HID_KEYBOARD_LED_CAPSLOCK,   &MacroEvent_CapsLockStateChanged  );
                processStateChange(&UsbBasicKeyboard_NumLockOn,    report->reportBuffer[0] & HID_KEYBOARD_LED_NUMLOCK,    &MacroEvent_NumLockStateChanged   );
                processStateChange(&UsbBasicKeyboard_ScrollLockOn, report->reportBuffer[0] & HID_KEYBOARD_LED_SCROLLLOCK, &MacroEvent_ScrollLockStateChanged);

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
                EventVector_Set(EventVector_ProtocolChanged);
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

void UsbBasicKeyboardSendActiveReport(void)
{
#ifdef __ZEPHYR__
    UsbCompatibility_SendKeyboardReport(ActiveUsbBasicKeyboardReport);
    SwitchActiveUsbBasicKeyboardReport();
#else
    UsbReportUpdateSemaphore |= 1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX;
    usb_status_t status = UsbBasicKeyboardAction();
    //The semaphore has to be set before the call. Assume what happens if a bus reset happens asynchronously here. (Deadlock.)
    if (status != kStatus_USB_Success) {
        //This is *not* asynchronously safe as long as multiple reports of different type can be sent at the same time.
        //TODO: consider either making it atomic, or lowering semaphore reset delay
        UsbReportUpdateSemaphore &= ~(1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX);
    }
#endif
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

static void setRolloverError(usb_basic_keyboard_report_t* report)
{
    if (report->boot.scancodes[0] != HID_KEYBOARD_SC_ERROR_ROLLOVER) {
        memset(report->boot.scancodes, HID_KEYBOARD_SC_ERROR_ROLLOVER, UTILS_ARRAY_SIZE(report->boot.scancodes));
    }
}

bool UsbBasicKeyboard_IsFullScancodes(const usb_basic_keyboard_report_t* report)
{
    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        return report->boot.scancodes[UTILS_ARRAY_SIZE(report->boot.scancodes) - 1] != 0;
    } else {
        return false;
    }
}

bool UsbBasicKeyboard_AddScancode(usb_basic_keyboard_report_t* report, uint8_t scancode)
{
    if (scancode == 0)
        return true;

    if (UsbBasicKeyboard_IsModifier(scancode)) {
        // modifiers are kept the same place in both report layouts
        set_bit(scancode - USB_BASIC_KEYBOARD_MIN_MODIFIERS_SCANCODE, &report->modifiers);
        return true;
    } else if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->boot.scancodes); i++) {
            if (report->boot.scancodes[i] == 0) {
                report->boot.scancodes[i] = scancode;
                return true;
            }
        }

        setRolloverError(report);
        return false;
    } else if (UsbBasicKeyboard_IsInBitfield(scancode)) {
        set_bit(scancode - USB_BASIC_KEYBOARD_MIN_BITFIELD_SCANCODE, report->bitfield);
        return true;
    } else {
        return false;
    }
}

void UsbBasicKeyboard_RemoveScancode(usb_basic_keyboard_report_t* report, uint8_t scancode)
{
    if (UsbBasicKeyboard_IsModifier(scancode)) {
        // modifiers are kept the same place in both report layouts
        clear_bit(scancode - USB_BASIC_KEYBOARD_MIN_MODIFIERS_SCANCODE, &report->modifiers);
    } else if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->boot.scancodes); i++) {
            if (report->boot.scancodes[i] == scancode) {
                report->boot.scancodes[i] = 0;
                return;
            }
        }
    } else if (UsbBasicKeyboard_IsInBitfield(scancode)) {
        clear_bit(scancode - USB_BASIC_KEYBOARD_MIN_BITFIELD_SCANCODE, report->bitfield);
    }
}

bool UsbBasicKeyboard_ContainsScancode(const usb_basic_keyboard_report_t* report, uint8_t scancode)
{
    if (UsbBasicKeyboard_IsModifier(scancode)) {
        // modifiers are kept the same place in both report layouts
        return test_bit(scancode - USB_BASIC_KEYBOARD_MIN_MODIFIERS_SCANCODE, &report->modifiers);
    } else if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->boot.scancodes); i++) {
            if (report->boot.scancodes[i] == scancode) {
                return true;
            }
        }
        return false;
    } else if (UsbBasicKeyboard_IsInBitfield(scancode)) {
        return test_bit(scancode - USB_BASIC_KEYBOARD_MIN_BITFIELD_SCANCODE, report->bitfield);
    } else {
        return false;
    }
}

size_t UsbBasicKeyboard_ScancodeCount(const usb_basic_keyboard_report_t* report)
{
    size_t size = 0;
    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        while ((size < UTILS_ARRAY_SIZE(report->boot.scancodes)) && (report->boot.scancodes[size] != 0)) {
            size++;
        }
    } else {
        for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->bitfield); i++) {
            for (uint8_t b = report->bitfield[i]; b > 0; b >>= 1) {
                size += (b & 1);
            }
        }
    }
    return size;
}

void UsbBasicKeyboard_MergeReports(const usb_basic_keyboard_report_t* sourceReport, usb_basic_keyboard_report_t* targetReport)
{
    targetReport->modifiers |= sourceReport->modifiers;

    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        uint8_t idx, i = 0;

        /* find empty position */
        for (idx = 0; idx < UTILS_ARRAY_SIZE(targetReport->boot.scancodes); idx++) {
            if (targetReport->boot.scancodes[idx] == 0) {
                break;
            }
        }
        /* copy into empty positions */
        while ((i < UTILS_ARRAY_SIZE(sourceReport->boot.scancodes)) && (sourceReport->boot.scancodes[i] != 0) && (idx < UTILS_ARRAY_SIZE(targetReport->boot.scancodes))) {
            targetReport->boot.scancodes[idx++] = sourceReport->boot.scancodes[i++];
        }

        /* target is full, but source isn't copied -> set error */
        if ((idx == UTILS_ARRAY_SIZE(targetReport->boot.scancodes)) && (i < UTILS_ARRAY_SIZE(sourceReport->boot.scancodes)) && (sourceReport->boot.scancodes[i] != 0)) {
            setRolloverError(targetReport);
        }
    } else {
        for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(targetReport->bitfield); i++) {
            targetReport->bitfield[i] |= sourceReport->bitfield[i];
        }
    }
}

void UsbBasicKeyboard_ForeachScancode(const usb_basic_keyboard_report_t* report, void(*action)(uint8_t))
{
    // TODO: do we need to consider modifiers?
    if (UsbBasicKeyboardGetProtocol() == USB_HID_BOOT_PROTOCOL) {
        for (uint8_t i = 0; (i < UTILS_ARRAY_SIZE(report->boot.scancodes)) && (report->boot.scancodes[i] != 0); i++) {
            action(report->boot.scancodes[i]);
        }
    } else {
        for (uint8_t i = 0; i < UTILS_ARRAY_SIZE(report->bitfield); i++) {
            for (uint8_t j = 0, b = report->bitfield[i]; b > 0; j++, b >>= 1) {
                if (b & 1) {
                    action(USB_BASIC_KEYBOARD_MIN_BITFIELD_SCANCODE + i * 8 + j);
                }
            }
        }
    }
}
