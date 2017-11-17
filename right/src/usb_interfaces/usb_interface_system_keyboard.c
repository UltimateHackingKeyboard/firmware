#include "key_action.h"
#include "fsl_port.h"
#include "usb_api.h"
#include "usb_composite_device.h"
#include "peripherals/test_led.h"
#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_addresses.h"

static usb_device_endpoint_struct_t UsbSystemKeyboardEndpoints[USB_SYSTEM_KEYBOARD_ENDPOINT_COUNT] = {{
    USB_SYSTEM_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_SYSTEM_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
}};

static usb_device_interface_struct_t UsbSystemKeyboardInterface[] = {{
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    {USB_SYSTEM_KEYBOARD_ENDPOINT_COUNT, UsbSystemKeyboardEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbSystemKeyboardInterfaces[USB_SYSTEM_KEYBOARD_INTERFACE_COUNT] = {{
    USB_CLASS_HID,
    USB_HID_SUBCLASS_BOOT,
    USB_HID_PROTOCOL_KEYBOARD,
    USB_SYSTEM_KEYBOARD_INTERFACE_INDEX,
    UsbSystemKeyboardInterface,
    sizeof(UsbSystemKeyboardInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbSystemKeyboardInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_SYSTEM_KEYBOARD_INTERFACE_COUNT,
    UsbSystemKeyboardInterfaces,
}};

usb_device_class_struct_t UsbSystemKeyboardClass = {
    UsbSystemKeyboardInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

static usb_system_keyboard_report_t usbSystemKeyboardReports[2];
usb_system_keyboard_report_t* ActiveUsbSystemKeyboardReport = usbSystemKeyboardReports;
bool IsUsbSystemKeyboardReportSent = false;

usb_system_keyboard_report_t* getInactiveUsbSystemKeyboardReport()
{
    return ActiveUsbSystemKeyboardReport == usbSystemKeyboardReports ? usbSystemKeyboardReports+1 : usbSystemKeyboardReports;
}

void SwitchActiveUsbSystemKeyboardReport(void)
{
    ActiveUsbSystemKeyboardReport = getInactiveUsbSystemKeyboardReport();
}

void ResetActiveUsbSystemKeyboardReport(void)
{
    bzero(ActiveUsbSystemKeyboardReport, USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
}

static usb_status_t UsbSystemKeyboardAction(void)
{
    usb_status_t status = USB_DeviceHidSend(
        UsbCompositeDevice.systemKeyboardHandle, USB_SYSTEM_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t*)getInactiveUsbSystemKeyboardReport(), USB_SYSTEM_KEYBOARD_REPORT_LENGTH);
    IsUsbSystemKeyboardReportSent = true;
    return status;
}

usb_status_t UsbSystemKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbSystemKeyboardAction();
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

usb_status_t UsbSystemKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbSystemKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbSystemKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_SYSTEM_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbSystemKeyboardAction();
    }
    return kStatus_USB_Error;
}
