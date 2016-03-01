#include "include/board/board.h"
#include "fsl_gpio.h"
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "include/usb/usb_device_ch9.h"
#include "composite.h"
#include "usb_class_mouse.h"
#include "usb_interface_mouse.h"
#include "usb_descriptor_configuration.h"

static usb_device_hid_mouse_struct_t UsbMouseDevice;

static usb_status_t UsbMouseAction(void)
{
    //usb_device_wheeled_mouse_struct_t *wheeledMouse = (usb_device_wheeled_mouse_struct_t*)&(UsbMouseDevice.buffer);
    UsbMouseDevice.buffer[0] = 0;
    UsbMouseDevice.buffer[1] = 0;
    UsbMouseDevice.buffer[2] = 0;
    UsbMouseDevice.buffer[3] = 0;
    UsbMouseDevice.buffer[4] = 0;
    UsbMouseDevice.buffer[5] = 0;
    UsbMouseDevice.buffer[6] = 0;
    if (!GPIO_ReadPinInput(BOARD_SW2_GPIO, BOARD_SW2_GPIO_PIN)) {
        UsbMouseDevice.buffer[1] = 1;
        //wheeledMouse->x = 32767;
        //wheeledMouse->y = 32767;
    }
    return USB_DeviceHidSend(UsbCompositeDevice.mouseHandle, USB_MOUSE_ENDPOINT_ID,
                             UsbMouseDevice.buffer, USB_MOUSE_REPORT_LENGTH);
}

usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbMouseAction();
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

usb_status_t UsbMouseSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbMouseAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_MOUSE_INTERFACE_INDEX == interface) {
        return UsbMouseAction();
    }
    return kStatus_USB_Error;
}
