#include "usb_composite_device.h"
#include "usb_protocol_handler.h"

usb_device_class_struct_t UsbGenericHidClass = {
    .type = kUSB_DeviceClassTypeHid,
    .configurations = USB_DEVICE_CONFIGURATION_COUNT,
    .interfaceList = (usb_device_interface_list_t [USB_DEVICE_CONFIGURATION_COUNT]) {{
        .count = USB_GENERIC_HID_INTERFACE_COUNT,
        .interfaces = (usb_device_interfaces_struct_t[USB_GENERIC_HID_INTERFACE_COUNT]) {{
            .classCode = USB_CLASS_HID,
            .subclassCode = USB_HID_SUBCLASS_NONE,
            .protocolCode = USB_HID_PROTOCOL_NONE,
            .interfaceNumber = USB_GENERIC_HID_INTERFACE_INDEX,
            .count = 1,
            .interface = (usb_device_interface_struct_t[]) {{
                .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
                .classSpecific = NULL,
                .endpointList = {
                    .count = USB_GENERIC_HID_ENDPOINT_COUNT,
                    .endpoint = (usb_device_endpoint_struct_t[USB_GENERIC_HID_ENDPOINT_COUNT]) {
                        {
                            .endpointAddress = USB_GENERIC_HID_ENDPOINT_IN_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
                            .transferType = USB_ENDPOINT_INTERRUPT,
                            .maxPacketSize = USB_GENERIC_HID_INTERRUPT_IN_PACKET_SIZE,
                        },
                        {
                            .endpointAddress = USB_GENERIC_HID_ENDPOINT_OUT_INDEX | (USB_OUT << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
                            .transferType = USB_ENDPOINT_INTERRUPT,
                            .maxPacketSize = USB_GENERIC_HID_INTERRUPT_OUT_PACKET_SIZE,
                        }
                    }
                }
            }}
        }}
    }}
};

uint32_t UsbGenericHidActionCounter;
uint8_t GenericHidInBuffer[USB_GENERIC_HID_IN_BUFFER_LENGTH];
uint8_t GenericHidOutBuffer[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

static usb_status_t UsbReceiveData(void)
{
    return USB_DeviceHidRecv(UsbCompositeDevice.genericHidHandle,
                             USB_GENERIC_HID_ENDPOINT_OUT_INDEX,
                             GenericHidInBuffer,
                             USB_GENERIC_HID_OUT_BUFFER_LENGTH);
}

usb_status_t UsbGenericHidCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            break;
        case kUSB_DeviceHidEventRecvResponse:
            UsbProtocolHandler();

            USB_DeviceHidSend(UsbCompositeDevice.genericHidHandle,
                              USB_GENERIC_HID_ENDPOINT_IN_INDEX,
                              GenericHidOutBuffer,
                              USB_GENERIC_HID_OUT_BUFFER_LENGTH);
            UsbGenericHidActionCounter++;
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
