#include "main.h"
#include "action.h"
#include "fsl_port.h"
#include "usb_api.h"
#include "usb_composite_device.h"
#include "peripherials/test_led.h"
#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_addresses.h"

static usb_device_endpoint_struct_t UsbBasicKeyboardEndpoints[USB_BASIC_KEYBOARD_ENDPOINT_COUNT] = {{
    USB_BASIC_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_BASIC_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
}};

static usb_device_interface_struct_t UsbBasicKeyboardInterface[] = {{
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    {USB_BASIC_KEYBOARD_ENDPOINT_COUNT, UsbBasicKeyboardEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbBasicKeyboardInterfaces[USB_BASIC_KEYBOARD_INTERFACE_COUNT] = {{
    USB_CLASS_HID,
    USB_HID_SUBCLASS_BOOT,
    USB_HID_PROTOCOL_KEYBOARD,
    USB_BASIC_KEYBOARD_INTERFACE_INDEX,
    UsbBasicKeyboardInterface,
    sizeof(UsbBasicKeyboardInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbBasicKeyboardInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_BASIC_KEYBOARD_INTERFACE_COUNT,
    UsbBasicKeyboardInterfaces,
}};

usb_device_class_struct_t UsbBasicKeyboardClass = {
    UsbBasicKeyboardInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

static usb_basic_keyboard_report_t usbBasicKeyboardReports[2];
usb_basic_keyboard_report_t* ActiveUsbBasicKeyboardReport = usbBasicKeyboardReports;
bool IsUsbBasicKeyboardReportSent = false;

usb_basic_keyboard_report_t* getInactiveUsbBasicKeyboardReport()
{
    return ActiveUsbBasicKeyboardReport == usbBasicKeyboardReports ? usbBasicKeyboardReports+1 : usbBasicKeyboardReports;
}

void SwitchActiveUsbBasicKeyboardReport()
{
    ActiveUsbBasicKeyboardReport = getInactiveUsbBasicKeyboardReport();
}

void ResetActiveUsbBasicKeyboardReport()
{
    bzero(ActiveUsbBasicKeyboardReport, USB_BASIC_KEYBOARD_REPORT_LENGTH);
}

static usb_status_t UsbBasicKeyboardAction(void)
{
    usb_status_t status = USB_DeviceHidSend(
        UsbCompositeDevice.basicKeyboardHandle, USB_BASIC_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t*)getInactiveUsbBasicKeyboardReport(), USB_BASIC_KEYBOARD_REPORT_LENGTH);
    IsUsbBasicKeyboardReportSent = true;
    return status;
}

usb_status_t UsbBasicKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbBasicKeyboardAction();
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

usb_status_t UsbBasicKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbBasicKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbBasicKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_BASIC_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbBasicKeyboardAction();
    }
    return kStatus_USB_Error;
}
