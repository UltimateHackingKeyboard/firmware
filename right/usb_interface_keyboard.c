#include "include/board/board.h"
#include "fsl_gpio.h"
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "include/usb/usb_device_ch9.h"
#include "composite.h"
#include "scancodes.h"
#include "usb_interface_keyboard.h"
#include "usb_class_keyboard.h"
#include "usb_descriptor_configuration.h"

static usb_device_hid_keyboard_struct_t UsbKeyboardDevice;

static usb_status_t UsbKeyboardAction(void)
{
    UsbKeyboardDevice.buffer[2] = 0x00U;
    if (!GPIO_ReadPinInput(BOARD_SW3_GPIO, BOARD_SW3_GPIO_PIN)) {
        UsbKeyboardDevice.buffer[2] = KEY_A;
    }
    return USB_DeviceHidSend(UsbCompositeDevice.keyboardHandle, USB_KEYBOARD_ENDPOINT_ID,
                             UsbKeyboardDevice.buffer, USB_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbKeyboardAction();
            }
            break;
        case kUSB_DeviceHidEventGetReport:
        case kUSB_DeviceHidEventSetReport:
        case kUSB_DeviceHidEventRequestReportBuffer:
            error = kStatus_USB_InvalidRequest;
            break;
        case kUSB_DeviceHidEventGetIdle:
        case kUSB_DeviceHidEventGetProtocol:
        case kUSB_DeviceHidEventSetIdle:
        case kUSB_DeviceHidEventSetProtocol:
            break;
        default:
            break;
    }

    return error;
}

usb_status_t UsbKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbKeyboardAction();
    }
    return kStatus_USB_Error;
}
