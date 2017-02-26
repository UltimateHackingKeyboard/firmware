#include "main.h"
#include "action.h"
#include "fsl_port.h"
#include "usb_api.h"
#include "usb_composite_device.h"
#include "peripherials/test_led.h"
#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_addresses.h"

static usb_device_endpoint_struct_t UsbKeyboardEndpoints[USB_KEYBOARD_ENDPOINT_COUNT] = {{
    USB_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
}};

static usb_device_interface_struct_t UsbKeyboardInterface[] = {{
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    {USB_KEYBOARD_ENDPOINT_COUNT, UsbKeyboardEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbKeyboardInterfaces[USB_KEYBOARD_INTERFACE_COUNT] = {{
    USB_CLASS_HID,
    USB_HID_SUBCLASS_BOOT,
    USB_HID_PROTOCOL_KEYBOARD,
    USB_KEYBOARD_INTERFACE_INDEX,
    UsbKeyboardInterface,
    sizeof(UsbKeyboardInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbKeyboardInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_KEYBOARD_INTERFACE_COUNT,
    UsbKeyboardInterfaces,
}};

usb_device_class_struct_t UsbKeyboardClass = {
    UsbKeyboardInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

static usb_keyboard_report_t usbKeyboardReports[2];
usb_keyboard_report_t* ActiveUsbKeyboardReport = usbKeyboardReports;
bool IsUsbKeyboardReportSent = false;

usb_keyboard_report_t* getInactiveUsbKeyboardReport()
{
    return ActiveUsbKeyboardReport == usbKeyboardReports ? usbKeyboardReports+1 : usbKeyboardReports;
}

void SwitchActiveUsbKeyboardReport()
{
    ActiveUsbKeyboardReport = getInactiveUsbKeyboardReport();
}

void ResetActiveUsbKeyboardReport()
{
    bzero(ActiveUsbKeyboardReport, USB_KEYBOARD_REPORT_LENGTH);
}

static usb_status_t UsbKeyboardAction(void)
{
    usb_status_t status = USB_DeviceHidSend(
        UsbCompositeDevice.keyboardHandle, USB_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t*)getInactiveUsbKeyboardReport(), USB_KEYBOARD_REPORT_LENGTH);
    IsUsbKeyboardReportSent = true;
    return status;
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
