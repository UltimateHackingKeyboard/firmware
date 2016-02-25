#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "include/usb/usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "hid_keyboard.h"

static usb_device_composite_struct_t *UsbCompositeDevice;
static usb_device_hid_keyboard_struct_t UsbKeyboardDevice;

static usb_status_t UsbKeyboardAction(void)
{
    static int x = 0U;
    enum {
        DOWN,
        UP
    };
    static uint8_t dir = DOWN;

    UsbKeyboardDevice.buffer[2] = 0x00U;
    switch (dir) {
        case DOWN:
            x++;
            if (x > 200U) {
                dir++;
                UsbKeyboardDevice.buffer[2] = KEY_PAGEUP;
            }
            break;
        case UP:
            x--;
            if (x < 1U) {
                dir = DOWN;
                UsbKeyboardDevice.buffer[2] = KEY_PAGEDOWN;
            }
            break;
        default:
            break;
    }
    return USB_DeviceHidSend(UsbCompositeDevice->hidKeyboardHandle, USB_KEYBOARD_ENDPOINT_ID,
                             UsbKeyboardDevice.buffer, USB_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice->attach) {
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

usb_status_t UsbKeyboardSetConfigure(class_handle_t handle, uint8_t configure)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configure) {
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

usb_status_t UsbKeyboardInit(usb_device_composite_struct_t *compositeDevice)
{
    UsbCompositeDevice = compositeDevice;
    return kStatus_USB_Success;
}
