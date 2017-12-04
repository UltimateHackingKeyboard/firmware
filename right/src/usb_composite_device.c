#include "usb_device_config.h"
#include "usb_composite_device.h"
#include "usb_descriptors/usb_descriptor_hid.h"
#include "usb_descriptors/usb_descriptor_strings.h"
#include "bus_pal_hardware.h"
#include "bootloader/wormhole.h"

usb_composite_device_t UsbCompositeDevice;
static usb_status_t UsbDeviceCallback(usb_device_handle handle, uint32_t event, void *param);

static usb_device_class_config_list_struct_t UsbDeviceCompositeConfigList = {
    .deviceCallback = UsbDeviceCallback,
    .count = USB_DEVICE_CONFIG_HID,
    .config = (usb_device_class_config_struct_t[USB_DEVICE_CONFIG_HID]) {{
        .classCallback = UsbGenericHidCallback,
        .classHandle = (class_handle_t)NULL,
        .classInfomation = (usb_device_class_struct_t[]) {{
            .type = kUSB_DeviceClassTypeHid,
            .configurations = USB_DEVICE_CONFIGURATION_COUNT,
            .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
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
        }}
    },
    {
        .classCallback = UsbBasicKeyboardCallback,
        .classHandle = (class_handle_t)NULL,
        .classInfomation = (usb_device_class_struct_t[]) {{
            .type = kUSB_DeviceClassTypeHid,
            .configurations = USB_DEVICE_CONFIGURATION_COUNT,
            .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
                .count = USB_BASIC_KEYBOARD_INTERFACE_COUNT,
                .interfaces = (usb_device_interfaces_struct_t[USB_BASIC_KEYBOARD_INTERFACE_COUNT]) {{
                    .classCode = USB_CLASS_HID,
                    .subclassCode = USB_HID_SUBCLASS_BOOT,
                    .protocolCode = USB_HID_PROTOCOL_KEYBOARD,
                    .interfaceNumber = USB_BASIC_KEYBOARD_INTERFACE_INDEX,
                    .count = 1,
                    .interface = (usb_device_interface_struct_t[]) {{
                        .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
                        .classSpecific = NULL,
                        .endpointList = {
                            USB_BASIC_KEYBOARD_ENDPOINT_COUNT,
                            (usb_device_endpoint_struct_t[USB_BASIC_KEYBOARD_ENDPOINT_COUNT]) {{
                                .endpointAddress = USB_BASIC_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
                                .transferType = USB_ENDPOINT_INTERRUPT,
                                .maxPacketSize = USB_BASIC_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
                            }}
                        }
                    }}
                }}
            }}
        }}
    },
    {
        .classCallback = UsbMediaKeyboardCallback,
        .classHandle = (class_handle_t)NULL,
        .classInfomation = (usb_device_class_struct_t[]) {{
            .type = kUSB_DeviceClassTypeHid,
            .configurations = USB_DEVICE_CONFIGURATION_COUNT,
            .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
                .count = USB_MEDIA_KEYBOARD_INTERFACE_COUNT,
                .interfaces = (usb_device_interfaces_struct_t[USB_MEDIA_KEYBOARD_INTERFACE_COUNT]) {{
                    .classCode = USB_CLASS_HID,
                    .subclassCode = USB_HID_SUBCLASS_BOOT,
                    .protocolCode = USB_HID_PROTOCOL_KEYBOARD,
                    .interfaceNumber = USB_MEDIA_KEYBOARD_INTERFACE_INDEX,
                    .count = 1,
                    .interface = (usb_device_interface_struct_t[]) {{
                        .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
                        .classSpecific = NULL,
                        .endpointList = {
                            USB_MEDIA_KEYBOARD_ENDPOINT_COUNT,
                            (usb_device_endpoint_struct_t[USB_MEDIA_KEYBOARD_ENDPOINT_COUNT]) {{
                                .endpointAddress = USB_MEDIA_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
                                .transferType = USB_ENDPOINT_INTERRUPT,
                                .maxPacketSize = USB_MEDIA_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
                            }}
                        }
                    }}
                }}
            }}
        }}
    },
    {
        .classCallback = UsbSystemKeyboardCallback,
        .classHandle = (class_handle_t)NULL,
        .classInfomation = (usb_device_class_struct_t[]) {{
            .type = kUSB_DeviceClassTypeHid,
            .configurations = USB_DEVICE_CONFIGURATION_COUNT,
            .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
                .count = USB_SYSTEM_KEYBOARD_INTERFACE_COUNT,
                .interfaces = (usb_device_interfaces_struct_t[USB_SYSTEM_KEYBOARD_INTERFACE_COUNT]) {{
                    .classCode = USB_CLASS_HID,
                    .subclassCode = USB_HID_SUBCLASS_BOOT,
                    .protocolCode = USB_HID_PROTOCOL_KEYBOARD,
                    .interfaceNumber = USB_SYSTEM_KEYBOARD_INTERFACE_INDEX,
                    .count = 1,
                    .interface = (usb_device_interface_struct_t[]) {{
                        .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
                        .classSpecific = NULL,
                        .endpointList = {
                            USB_SYSTEM_KEYBOARD_ENDPOINT_COUNT,
                            (usb_device_endpoint_struct_t[USB_SYSTEM_KEYBOARD_ENDPOINT_COUNT]) {{
                                .endpointAddress = USB_SYSTEM_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
                                .transferType = USB_ENDPOINT_INTERRUPT,
                                .maxPacketSize = USB_SYSTEM_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
                            }}
                        }
                    }}
                }}
            }},
        }}
    },
    {
        .classCallback = UsbMouseCallback,
        .classHandle = (class_handle_t)NULL,
        .classInfomation = (usb_device_class_struct_t[]) {{
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
        }}
    }
}};

static usb_status_t UsbDeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    uint16_t *temp16 = (uint16_t*)param;
    uint8_t *temp8 = (uint8_t*)param;

    if (!param && event != kUSB_DeviceEventBusReset && event != kUSB_DeviceEventSetInterface) {
        return error;
    }

    switch (event) {
        case kUSB_DeviceEventBusReset:
            UsbCompositeDevice.attach = 0;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSetConfiguration:
            UsbCompositeDevice.attach = 1;
            UsbCompositeDevice.currentConfiguration = *temp8;
            UsbGenericHidSetConfiguration(UsbCompositeDevice.genericHidHandle, *temp8);
            UsbBasicKeyboardSetConfiguration(UsbCompositeDevice.basicKeyboardHandle, *temp8);
            UsbMediaKeyboardSetConfiguration(UsbCompositeDevice.mediaKeyboardHandle, *temp8);
            UsbSystemKeyboardSetConfiguration(UsbCompositeDevice.systemKeyboardHandle, *temp8);
            UsbMouseSetConfiguration(UsbCompositeDevice.mouseHandle, *temp8);
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventGetConfiguration:
            *temp8 = UsbCompositeDevice.currentConfiguration;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSetInterface:
            if (UsbCompositeDevice.attach) {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 8);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FF);
                if (interface < USB_DEVICE_CONFIG_HID) {
                    UsbCompositeDevice.currentInterfaceAlternateSetting[interface] = alternateSetting;
                    UsbGenericHidSetInterface(UsbCompositeDevice.genericHidHandle, interface, alternateSetting);
                    UsbBasicKeyboardSetInterface(UsbCompositeDevice.basicKeyboardHandle, interface, alternateSetting);
                    UsbMediaKeyboardSetInterface(UsbCompositeDevice.mediaKeyboardHandle, interface, alternateSetting);
                    UsbSystemKeyboardSetInterface(UsbCompositeDevice.systemKeyboardHandle, interface, alternateSetting);
                    UsbMouseSetInterface(UsbCompositeDevice.mouseHandle, interface, alternateSetting);
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetInterface: ;
            uint8_t interface = (uint8_t)((*temp16 & 0xFF00) >> 8);
            if (interface < USB_DEVICE_CONFIG_HID) {
                *temp16 = (*temp16 & 0xFF00) | UsbCompositeDevice.currentInterfaceAlternateSetting[interface];
                error = kStatus_USB_Success;
            } else {
                error = kStatus_USB_InvalidRequest;
            }
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            error = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            error = USB_DeviceGetConfigurationDescriptor(handle, (usb_device_get_configuration_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetStringDescriptor:
            error = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetHidDescriptor:
            error = USB_DeviceGetHidDescriptor(handle, (usb_device_get_hid_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetHidReportDescriptor:
            error = USB_DeviceGetHidReportDescriptor(handle, (usb_device_get_hid_report_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetHidPhysicalDescriptor:
            error = USB_DeviceGetHidPhysicalDescriptor(handle, (usb_device_get_hid_physical_descriptor_struct_t *)param);
            break;
    }

    return error;
}

void USB0_IRQHandler(void)
{
    USB_DeviceKhciIsrFunction(Wormhole.enumerationMode == EnumerationMode_BusPal
        ? BuspalCompositeUsbDevice.device_handle
        : UsbCompositeDevice.deviceHandle);
}

void InitUsb(void)
{
    uint8_t usbDeviceKhciIrq[] = USB_IRQS;
    uint8_t irqNumber = usbDeviceKhciIrq[CONTROLLER_ID - kUSB_ControllerKhci0];

    SystemCoreClockUpdate();
    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, 48000000);

    UsbCompositeDevice.attach = 0;
    USB_DeviceClassInit(CONTROLLER_ID, &UsbDeviceCompositeConfigList, &UsbCompositeDevice.deviceHandle);
    UsbCompositeDevice.genericHidHandle = UsbDeviceCompositeConfigList.config[USB_GENERIC_HID_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.basicKeyboardHandle = UsbDeviceCompositeConfigList.config[USB_BASIC_KEYBOARD_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.mediaKeyboardHandle = UsbDeviceCompositeConfigList.config[USB_MEDIA_KEYBOARD_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.systemKeyboardHandle = UsbDeviceCompositeConfigList.config[USB_SYSTEM_KEYBOARD_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.mouseHandle = UsbDeviceCompositeConfigList.config[USB_MOUSE_INTERFACE_INDEX].classHandle;

    NVIC_EnableIRQ((IRQn_Type)irqNumber);

    USB_DeviceRun(UsbCompositeDevice.deviceHandle);
}
