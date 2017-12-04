#include "usb_composite_device.h"

usb_device_class_struct_t UsbMouseClass = {
    .type = kUSB_DeviceClassTypeHid,
    .configurations = USB_DEVICE_CONFIGURATION_COUNT,
    .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
        .count = USB_MOUSE_INTERFACE_COUNT,
        .interfaces = (usb_device_interfaces_struct_t[USB_MOUSE_INTERFACE_COUNT]) {{
            .classCode = USB_CLASS_HID,
            .subclassCode = USB_HID_SUBCLASS_BOOT,
            .protocolCode = USB_HID_PROTOCOL_MOUSE,
            .interfaceNumber = USB_MOUSE_INTERFACE_INDEX,
            .count = 1,
            .interface = (usb_device_interface_struct_t[]) {{
                .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
                .classSpecific = NULL,
                .endpointList = {
                    USB_MOUSE_ENDPOINT_COUNT,
                    (usb_device_endpoint_struct_t[USB_MOUSE_ENDPOINT_COUNT]) {{
                        .endpointAddress = USB_MOUSE_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
                        .transferType = USB_ENDPOINT_INTERRUPT,
                        .maxPacketSize = USB_MOUSE_INTERRUPT_IN_PACKET_SIZE,
                    }}
                }
            }}
        }}
    }}
};

uint32_t UsbMouseActionCounter;
static usb_mouse_report_t usbMouseReports[2];
usb_mouse_report_t* ActiveUsbMouseReport = usbMouseReports;
bool IsUsbMouseReportSent = false;

usb_mouse_report_t* getInactiveUsbMouseReport(void)
{
    return ActiveUsbMouseReport == usbMouseReports ? usbMouseReports+1 : usbMouseReports;
}

void SwitchActiveUsbMouseReport(void)
{
    ActiveUsbMouseReport = getInactiveUsbMouseReport();
}

void ResetActiveUsbMouseReport(void)
{
    bzero(ActiveUsbMouseReport, USB_MOUSE_REPORT_LENGTH);
}

static volatile usb_status_t usbMouseAction(void)
{
    usb_mouse_report_t *mouseReport = getInactiveUsbMouseReport();
    SetDebugBufferUint16(61, mouseReport->x);
    SetDebugBufferUint16(63, mouseReport->y);
    IsUsbMouseReportSent = true;
    return USB_DeviceHidSend(UsbCompositeDevice.mouseHandle, USB_MOUSE_ENDPOINT_INDEX,
               (uint8_t*)mouseReport, USB_MOUSE_REPORT_LENGTH);
}

usb_status_t UsbMouseCallback(class_handle_t handle, uint32_t event, void *param)
{
    UsbMouseActionCounter++;
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return usbMouseAction();
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
        return usbMouseAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbMouseSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_MOUSE_INTERFACE_INDEX == interface) {
        return usbMouseAction();
    }
    return kStatus_USB_Error;
}
