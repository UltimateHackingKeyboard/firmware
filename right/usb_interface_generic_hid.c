#include "include/board/board.h"
#include "fsl_gpio.h"
#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "include/usb/usb_device_ch9.h"
#include "usb_interface_generic_hid.h"
#include "usb_class_generic_hid.h"
#include "usb_descriptor_configuration.h"
#include "composite.h"
#include "scancodes.h"

static usb_device_generic_hid_struct_t UsbGenericHidDevice;

static usb_status_t UsbReceiveData()
{
    return USB_DeviceHidRecv(UsbCompositeDevice.genericHidHandle, USB_GENERIC_HID_ENDPOINT_OUT_ID,
                             (uint8_t *)&UsbGenericHidDevice.buffer[UsbGenericHidDevice.bufferIndex][0],
                             USB_GENERIC_HID_OUT_BUFFER_LENGTH);
}

usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            break;
        case kUSB_DeviceHidEventRecvResponse:
            GPIO_SetPinsOutput(BOARD_LED_RED_GPIO, 1U << BOARD_LED_RED_GPIO_PIN);
            GPIO_SetPinsOutput(BOARD_LED_GREEN_GPIO, 1U << BOARD_LED_GREEN_GPIO_PIN);
            GPIO_SetPinsOutput(BOARD_LED_BLUE_GPIO, 1U << BOARD_LED_BLUE_GPIO_PIN);

            uint8_t command = (uint8_t)UsbGenericHidDevice.buffer[UsbGenericHidDevice.bufferIndex][0];

            switch (command) {
                case 'r':
                    GPIO_ClearPinsOutput(BOARD_LED_RED_GPIO, 1U << BOARD_LED_RED_GPIO_PIN);
                    break;
                case 'g':
                    GPIO_ClearPinsOutput(BOARD_LED_GREEN_GPIO, 1U << BOARD_LED_GREEN_GPIO_PIN);
                    break;
                case 'b':
                    GPIO_ClearPinsOutput(BOARD_LED_BLUE_GPIO, 1U << BOARD_LED_BLUE_GPIO_PIN);
                    break;
            }

            USB_DeviceHidSend(UsbCompositeDevice.genericHidHandle, USB_GENERIC_HID_ENDPOINT_IN_ID,
                              (uint8_t *)&UsbGenericHidDevice.buffer[UsbGenericHidDevice.bufferIndex][0],
                              USB_GENERIC_HID_OUT_BUFFER_LENGTH);
            UsbGenericHidDevice.bufferIndex ^= 1U;
            return UsbReceiveData();
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

usb_status_t UsbGenericHidSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbReceiveData();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbGenericHidSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_GENERIC_HID_INTERFACE_INDEX == interface) {
        return UsbReceiveData();
    }
    return kStatus_USB_Error;
}
