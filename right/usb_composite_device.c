#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "include/usb/usb_device_ch9.h"
#include "usb_descriptor_device.h"
#include "usb_composite_device.h"
#include "usb_interface_keyboard.h"
#include "usb_interface_mouse.h"
#include "usb_interface_generic_hid.h"
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include <stdio.h>
#include <stdlib.h>
#include "fsl_common.h"
#include "usb_descriptor_strings.h"
#include "usb_descriptor_hid.h"

static usb_status_t UsbDeviceCallback(usb_device_handle handle, uint32_t event, void *param);
usb_composite_device_t UsbCompositeDevice;

usb_device_class_config_struct_t UsbDeviceCompositeClassConfig[USB_COMPOSITE_INTERFACE_COUNT] = {
    {UsbGenericHidCallback, (class_handle_t)NULL, &UsbGenericHidClass},
    {UsbKeyboardCallback,   (class_handle_t)NULL, &UsbKeyboardClass},
    {UsbMouseCallback,      (class_handle_t)NULL, &UsbMouseClass},
};

usb_device_class_config_list_struct_t UsbDeviceCompositeConfigList = {
    UsbDeviceCompositeClassConfig,
    UsbDeviceCallback,
    USB_COMPOSITE_INTERFACE_COUNT
};

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
            UsbKeyboardSetConfiguration(UsbCompositeDevice.keyboardHandle, *temp8);
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
                if (interface < USB_COMPOSITE_INTERFACE_COUNT) {
                    UsbCompositeDevice.currentInterfaceAlternateSetting[interface] = alternateSetting;
                    UsbGenericHidSetInterface(UsbCompositeDevice.genericHidHandle, interface, alternateSetting);
                    UsbKeyboardSetInterface(UsbCompositeDevice.keyboardHandle, interface, alternateSetting);
                    UsbMouseSetInterface(UsbCompositeDevice.mouseHandle, interface, alternateSetting);
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetInterface: ;
            uint8_t interface = (uint8_t)((*temp16 & 0xFF00) >> 8);
            if (interface < USB_COMPOSITE_INTERFACE_COUNT) {
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
    USB_DeviceKhciIsrFunction(UsbCompositeDevice.deviceHandle);
}

void USB_DeviceApplicationInit(void)
{
    uint8_t usbDeviceKhciIrq[] = USB_IRQS;
    uint8_t irqNumber = usbDeviceKhciIrq[CONTROLLER_ID - kUSB_ControllerKhci0];

    SystemCoreClockUpdate();
    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, 48000000);

    UsbCompositeDevice.attach = 0;
    USB_DeviceClassInit(CONTROLLER_ID, &UsbDeviceCompositeConfigList, &UsbCompositeDevice.deviceHandle);
    UsbCompositeDevice.genericHidHandle = UsbDeviceCompositeConfigList.config[USB_GENERIC_HID_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.keyboardHandle = UsbDeviceCompositeConfigList.config[USB_KEYBOARD_INTERFACE_INDEX].classHandle;
    UsbCompositeDevice.mouseHandle = UsbDeviceCompositeConfigList.config[USB_MOUSE_INTERFACE_INDEX].classHandle;

    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ((IRQn_Type)irqNumber);

    USB_DeviceRun(UsbCompositeDevice.deviceHandle);
}
