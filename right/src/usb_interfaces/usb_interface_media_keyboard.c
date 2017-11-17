#include "key_action.h"
#include "fsl_port.h"
#include "usb_api.h"
#include "usb_composite_device.h"
#include "peripherals/test_led.h"
#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_addresses.h"

static usb_device_endpoint_struct_t UsbMediaKeyboardEndpoints[USB_MEDIA_KEYBOARD_ENDPOINT_COUNT] = {{
    USB_MEDIA_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_MEDIA_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
}};

static usb_device_interface_struct_t UsbMediaKeyboardInterface[] = {{
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    {USB_MEDIA_KEYBOARD_ENDPOINT_COUNT, UsbMediaKeyboardEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbMediaKeyboardInterfaces[USB_MEDIA_KEYBOARD_INTERFACE_COUNT] = {{
    USB_CLASS_HID,
    USB_HID_SUBCLASS_BOOT,
    USB_HID_PROTOCOL_KEYBOARD,
    USB_MEDIA_KEYBOARD_INTERFACE_INDEX,
    UsbMediaKeyboardInterface,
    sizeof(UsbMediaKeyboardInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbMediaKeyboardInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_MEDIA_KEYBOARD_INTERFACE_COUNT,
    UsbMediaKeyboardInterfaces,
}};

usb_device_class_struct_t UsbMediaKeyboardClass = {
    UsbMediaKeyboardInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

static usb_media_keyboard_report_t usbMediaKeyboardReports[2];
usb_media_keyboard_report_t* ActiveUsbMediaKeyboardReport = usbMediaKeyboardReports;
bool IsUsbMediaKeyboardReportSent = false;

usb_media_keyboard_report_t* getInactiveUsbMediaKeyboardReport(void)
{
    return ActiveUsbMediaKeyboardReport == usbMediaKeyboardReports ? usbMediaKeyboardReports+1 : usbMediaKeyboardReports;
}

void SwitchActiveUsbMediaKeyboardReport(void)
{
    ActiveUsbMediaKeyboardReport = getInactiveUsbMediaKeyboardReport();
}

void ResetActiveUsbMediaKeyboardReport(void)
{
    bzero(ActiveUsbMediaKeyboardReport, USB_MEDIA_KEYBOARD_REPORT_LENGTH);
}

static usb_status_t UsbMediaKeyboardAction(void)
{
    usb_status_t status = USB_DeviceHidSend(
        UsbCompositeDevice.mediaKeyboardHandle, USB_MEDIA_KEYBOARD_ENDPOINT_INDEX,
        (uint8_t*)getInactiveUsbMediaKeyboardReport(), USB_MEDIA_KEYBOARD_REPORT_LENGTH);
    IsUsbMediaKeyboardReportSent = true;
    return status;
}

usb_status_t UsbMediaKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbMediaKeyboardAction();
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

usb_status_t UsbMediaKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbMediaKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbMediaKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_MEDIA_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbMediaKeyboardAction();
    }
    return kStatus_USB_Error;
}
