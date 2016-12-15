#include "usb_composite_device.h"
#include "usb_interface_mouse.h"
#include "fsl_i2c.h"
#include "i2c.h"
#include "reset_button.h"
#include "action.h"

static usb_device_endpoint_struct_t UsbMouseEndpoints[USB_MOUSE_ENDPOINT_COUNT] = {{
    USB_MOUSE_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_MOUSE_INTERRUPT_IN_PACKET_SIZE,
}};

static usb_device_interface_struct_t UsbMouseInterface[] = {{
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    {USB_MOUSE_ENDPOINT_COUNT, UsbMouseEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbMouseInterfaces[USB_MOUSE_INTERFACE_COUNT] = {{
    USB_CLASS_HID,
    USB_HID_SUBCLASS_BOOT,
    USB_HID_PROTOCOL_MOUSE,
    USB_MOUSE_INTERFACE_INDEX,
    UsbMouseInterface,
    sizeof(UsbMouseInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbMouseInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_MOUSE_INTERFACE_COUNT,
    UsbMouseInterfaces,
}};

usb_device_class_struct_t UsbMouseClass = {
    UsbMouseInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

usb_mouse_report_t UsbMouseReport;

static volatile usb_status_t UsbMouseAction(void)
{
    usb_status_t ret;
    ret = USB_DeviceHidSend(UsbCompositeDevice.mouseHandle, USB_MOUSE_ENDPOINT_INDEX,
                            (uint8_t*)&UsbMouseReport, USB_MOUSE_REPORT_LENGTH);
    UsbMouseReport.buttons = 0;
    UsbMouseReport.x = 0;
    UsbMouseReport.y = 0;
    UsbMouseReport.wheelX = 0;
    UsbMouseReport.wheelY = 0;

    return ret;
}

void fillMouseReport(uhk_key_t key, const uint8_t *prevKeyStates, const uint8_t *currKeyStates, uint8_t keyId)
{
    HandleMouseKey(&UsbMouseReport, key, prevKeyStates, currKeyStates, keyId);
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
