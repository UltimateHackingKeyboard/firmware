#include "include/board/board.h"
#include "usb_api.h"
#include "usb_composite_device.h"

static usb_device_endpoint_struct_t UsbGenericHidEndpoints[USB_GENERIC_HID_ENDPOINT_COUNT] =
{
    {
        USB_GENERIC_HID_ENDPOINT_IN_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_INTERRUPT,
        USB_GENERIC_HID_INTERRUPT_IN_PACKET_SIZE,
    },
    {
        USB_GENERIC_HID_ENDPOINT_OUT_INDEX | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
        USB_ENDPOINT_INTERRUPT,
        USB_GENERIC_HID_INTERRUPT_OUT_PACKET_SIZE,
    }
};

static usb_device_interface_struct_t UsbGenericHidInterface[] = {{
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    {USB_GENERIC_HID_ENDPOINT_COUNT, UsbGenericHidEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbGenericHidInterfaces[USB_GENERIC_HID_INTERFACE_COUNT] = {{
    USB_CLASS_HID,
    USB_HID_SUBCLASS_NONE,
    USB_HID_PROTOCOL_NONE,
    USB_GENERIC_HID_INTERFACE_INDEX,
    UsbGenericHidInterface,
    sizeof(UsbGenericHidInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbGenericHidInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_GENERIC_HID_INTERFACE_COUNT,
    UsbGenericHidInterfaces,
}};

usb_device_class_struct_t UsbGenericHidClass = {
    UsbGenericHidInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

static uint8_t GenericHidBuffer[2][USB_GENERIC_HID_IN_BUFFER_LENGTH];
static uint8_t GenericHidBufferIndex;

static usb_status_t UsbReceiveData()
{
    return USB_DeviceHidRecv(UsbCompositeDevice.genericHidHandle, USB_GENERIC_HID_ENDPOINT_OUT_INDEX,
                             &GenericHidBuffer[GenericHidBufferIndex][0],
                             USB_GENERIC_HID_OUT_BUFFER_LENGTH);
}

usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            break;
        case kUSB_DeviceHidEventRecvResponse:
            GPIO_SetPinsOutput(BOARD_LED_RED_GPIO, 1 << BOARD_LED_RED_GPIO_PIN);
            GPIO_SetPinsOutput(BOARD_LED_GREEN_GPIO, 1 << BOARD_LED_GREEN_GPIO_PIN);
            GPIO_SetPinsOutput(BOARD_LED_BLUE_GPIO, 1 << BOARD_LED_BLUE_GPIO_PIN);

            uint8_t command = GenericHidBuffer[GenericHidBufferIndex][0];

            switch (command) {
                case 'r':
                    GPIO_ClearPinsOutput(BOARD_LED_RED_GPIO, 1 << BOARD_LED_RED_GPIO_PIN);
                    break;
                case 'g':
                    GPIO_ClearPinsOutput(BOARD_LED_GREEN_GPIO, 1 << BOARD_LED_GREEN_GPIO_PIN);
                    break;
                case 'b':
                    GPIO_ClearPinsOutput(BOARD_LED_BLUE_GPIO, 1 << BOARD_LED_BLUE_GPIO_PIN);
                    break;
            }

            USB_DeviceHidSend(UsbCompositeDevice.genericHidHandle, USB_GENERIC_HID_ENDPOINT_IN_INDEX,
                              &GenericHidBuffer[GenericHidBufferIndex][0],
                              USB_GENERIC_HID_OUT_BUFFER_LENGTH);
            GenericHidBufferIndex ^= 1U;
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
