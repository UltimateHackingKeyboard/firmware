#include "config.h"
#include "led_display.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "usb_device_config.h"
#include "usb_composite_device.h"
#include "usb_descriptors/usb_descriptor_hid.h"
#include "usb_descriptors/usb_descriptor_strings.h"
#include "usb_descriptors/usb_descriptors_microsoft.h"
#include "usb_microsoft_os.h"
#include "bus_pal_hardware.h"
#include "bootloader/wormhole.h"

static uint8_t MsAltEnumMode = 0;
usb_composite_device_t UsbCompositeDevice;
static usb_status_t usbDeviceCallback(usb_device_handle handle, uint32_t event, void *param);

static usb_device_class_config_list_struct_t UsbDeviceCompositeConfigList = {
    .deviceCallback = usbDeviceCallback,
    .count = USB_DEVICE_CONFIG_HID,
    .config = (usb_device_class_config_struct_t[USB_DEVICE_CONFIG_HID]) {
    {
        .classCallback = UsbGenericHidCallback,
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
        .classInfomation = (usb_device_class_struct_t[]) {{
            .type = kUSB_DeviceClassTypeHid,
            .configurations = USB_DEVICE_CONFIGURATION_COUNT,
            .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
                .count = USB_MEDIA_KEYBOARD_INTERFACE_COUNT,
                .interfaces = (usb_device_interfaces_struct_t[USB_MEDIA_KEYBOARD_INTERFACE_COUNT]) {{
                    .classCode = USB_CLASS_HID,
                    .subclassCode = USB_HID_SUBCLASS_NONE,
                    .protocolCode = USB_HID_PROTOCOL_NONE,
                    .interfaceNumber = USB_MEDIA_KEYBOARD_INTERFACE_INDEX,
                    .count = 1,
                    .interface = (usb_device_interface_struct_t[]) {{
                        .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
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
        .classInfomation = (usb_device_class_struct_t[]) {{
            .type = kUSB_DeviceClassTypeHid,
            .configurations = USB_DEVICE_CONFIGURATION_COUNT,
            .interfaceList = (usb_device_interface_list_t[USB_DEVICE_CONFIGURATION_COUNT]) {{
                .count = USB_SYSTEM_KEYBOARD_INTERFACE_COUNT,
                .interfaces = (usb_device_interfaces_struct_t[USB_SYSTEM_KEYBOARD_INTERFACE_COUNT]) {{
                    .classCode = USB_CLASS_HID,
                    .subclassCode = USB_HID_SUBCLASS_NONE,
                    .protocolCode = USB_HID_PROTOCOL_NONE,
                    .interfaceNumber = USB_SYSTEM_KEYBOARD_INTERFACE_INDEX,
                    .count = 1,
                    .interface = (usb_device_interface_struct_t[]) {{
                        .alternateSetting = USB_INTERFACE_ALTERNATE_SETTING_NONE,
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
    },
}};

volatile bool SleepModeActive = true;
static volatile bool wakeUpHostAllowed;

static void suspendUhk(void) {
    SleepModeActive = true;
    LedSlaveDriver_DisableLeds();
}

static void wakeUpUhk(void) {
    SleepModeActive = false;
    LedSlaveDriver_UpdateLeds();
}

void WakeUpHost(void) {
    if (!wakeUpHostAllowed) {
        return;
    }
    // Send resume signal - this will call USB_DeviceKhciControl(khciHandle, kUSB_DeviceControlResume, NULL);
    USB_DeviceSetStatus(UsbCompositeDevice.deviceHandle, kUSB_DeviceStatusBus, NULL);
    while (SleepModeActive) {
        ;
    }
}

static usb_status_t usbDeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t status = kStatus_USB_Error;
    uint16_t *temp16 = (uint16_t*)param;
    uint8_t *temp8 = (uint8_t*)param;

    if (!param && event != kUSB_DeviceEventBusReset && event != kUSB_DeviceEventSetInterface && event != kUSB_DeviceEventSuspend && event != kUSB_DeviceEventResume) {
        return status;
    }

    switch (event) {
        case kUSB_DeviceEventBusReset:
            UsbCompositeDevice.attach = 0;
            MsAltEnumMode = 0;
            status = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSuspend:
            if (UsbCompositeDevice.attach) {
                suspendUhk(); // The host sends this event when it goes to sleep, so turn off all the LEDs.
                status = kStatus_USB_Success;
            }
            break;
        case kUSB_DeviceEventResume:
            wakeUpUhk();
            status = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSetConfiguration: {
            uint8_t interface;
            UsbCompositeDevice.attach = 1;
            wakeUpUhk();
            for (interface = 0; interface < USB_DEVICE_CONFIG_HID; ++interface) {
                usb_device_class_config_struct_t *intf = &UsbDeviceCompositeConfigList.config[interface];

                /* event enums collide with HID ones, so invert the value */
                status |= intf->classCallback(intf->classHandle, (uint32_t)-kUSB_DeviceEventSetConfiguration, temp8);
            }
            UsbCompositeDevice.currentConfiguration = *temp8;
            break;
        }
        case kUSB_DeviceEventGetConfiguration:
            *temp8 = UsbCompositeDevice.currentConfiguration;
            status = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSetInterface:
            if (UsbCompositeDevice.attach) {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 8);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FF);
                if (interface < USB_DEVICE_CONFIG_HID) {
                    usb_device_class_config_struct_t *intf = &UsbDeviceCompositeConfigList.config[interface];

                    /* event enums collide with HID ones, so invert the value */
                    status = intf->classCallback(intf->classHandle, (uint32_t)-kUSB_DeviceEventSetInterface, &alternateSetting);
                    UsbCompositeDevice.currentInterfaceAlternateSetting[interface] = alternateSetting;
                }
            }
            break;
        case kUSB_DeviceEventGetInterface: ;
            uint8_t interface = (uint8_t)((*temp16 & 0xFF00) >> 8);
            if (interface < USB_DEVICE_CONFIG_HID) {
                *temp16 = (*temp16 & 0xFF00) | UsbCompositeDevice.currentInterfaceAlternateSetting[interface];
                status = kStatus_USB_Success;
            } else {
                status = kStatus_USB_InvalidRequest;
            }
            break;
        case kUSB_DeviceEventGetDeviceDescriptor:
            status = USB_DeviceGetDeviceDescriptor(handle, (usb_device_get_device_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetConfigurationDescriptor:
            status = USB_DeviceGetConfigurationDescriptor(handle, (usb_device_get_configuration_descriptor_struct_t *)param,
                    MsAltEnumMode);
            break;
        case kUSB_DeviceEventGetStringDescriptor:
            status = USB_DeviceGetStringDescriptor(handle, (usb_device_get_string_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetHidDescriptor:
            status = USB_DeviceGetHidDescriptor(handle, (usb_device_get_hid_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetHidReportDescriptor:
            status = USB_DeviceGetHidReportDescriptor(handle, (usb_device_get_hid_report_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventGetHidPhysicalDescriptor:
            status = USB_DeviceGetHidPhysicalDescriptor(handle, (usb_device_get_hid_physical_descriptor_struct_t *)param);
            break;
        case kUSB_DeviceEventSetRemoteWakeup:
            wakeUpHostAllowed = *temp8;
            status = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventGetDeviceStatus:
            if (wakeUpHostAllowed)
                *temp16 |= (USB_DEVICE_CONFIG_REMOTE_WAKEUP << (USB_REQUSET_STANDARD_GET_STATUS_DEVICE_REMOTE_WARKUP_SHIFT));
            status = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventVendorRequest: ;
#if (USBD_MS_OS_DESC_VERSION == 2)
            usb_device_control_request_struct_t *controlRequest = (usb_device_control_request_struct_t *)param;

            if (controlRequest->setup->bRequest == USB_REQ_MICROSOFT_OS) {
                if (controlRequest->setup->wIndex == USB_MS_OS_2p0_GET_DESCRIPTOR_INDEX) {
                    status = USB_DeviceGetMsOsDescriptor(handle, controlRequest);
                }
                else if (controlRequest->setup->wIndex == USB_MS_OS_2p0_SET_ALT_ENUMERATION_INDEX) {
                    MsAltEnumMode = controlRequest->setup->wValue >> 8;
                    status = kStatus_USB_Success;
                }
            }
#endif /* (USBD_MS_OS_DESC_VERSION == 2) */
            break;
    }

    return status;
}

void USB0_IRQHandler(void)
{
    USB_DeviceKhciIsrFunction(Wormhole.enumerationMode == EnumerationMode_BusPal
        ? BuspalCompositeUsbDevice.device_handle
        : UsbCompositeDevice.deviceHandle);
}

void InitUsb(void)
{
    SystemCoreClockUpdate();
    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, 48000000);

    UsbCompositeDevice.attach = 0;
    USB_DeviceClassInit(CONTROLLER_ID, &UsbDeviceCompositeConfigList, &UsbCompositeDevice.deviceHandle);
    UsbCompositeDevice.genericHidHandle = UsbDeviceCompositeConfigList.config[USB_GENERIC_HID_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.basicKeyboardHandle = UsbDeviceCompositeConfigList.config[USB_BASIC_KEYBOARD_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.mediaKeyboardHandle = UsbDeviceCompositeConfigList.config[USB_MEDIA_KEYBOARD_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.systemKeyboardHandle = UsbDeviceCompositeConfigList.config[USB_SYSTEM_KEYBOARD_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.mouseHandle = UsbDeviceCompositeConfigList.config[USB_MOUSE_INTERFACE_INDEX].classHandle;

    uint8_t usbDeviceKhciIrq[] = USB_IRQS;
    uint8_t irqNumber = usbDeviceKhciIrq[CONTROLLER_ID - kUSB_ControllerKhci0];
    NVIC_EnableIRQ((IRQn_Type)irqNumber);

    FMC->PFAPR |= (1 << FMC_PFAPR_M3AP_SHIFT) | (1 << FMC_PFAPR_M4AP_SHIFT); // allow USB controller to read from Flash
    USB_DeviceRun(UsbCompositeDevice.deviceHandle);
}
