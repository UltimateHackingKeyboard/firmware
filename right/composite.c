#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "include/usb/usb_device_ch9.h"
#include "usb_descriptor_device.h"
#include "composite.h"
#include "usb_interface_keyboard.h"
#include "usb_interface_mouse.h"
#include "usb_interface_generic_hid.h"
#include "fsl_device_registers.h"
#include "include/board/clock_config.h"
#include "include/board/board.h"
#include "fsl_debug_console.h"
#include <stdio.h>
#include <stdlib.h>
#include "fsl_common.h"
#include "include/board/pin_mux.h"
#include "usb_descriptor_strings.h"

static usb_status_t UsbDeviceCallback(usb_device_handle handle, uint32_t event, void *param);
usb_device_composite_struct_t UsbCompositeDevice;

usb_device_class_config_struct_t UsbDeviceCompositeClassConfig[USB_COMPOSITE_INTERFACE_COUNT] = {
    {UsbKeyboardCallback,   (class_handle_t)NULL, &UsbKeyboardClass},
    {UsbMouseCallback,      (class_handle_t)NULL, &UsbMouseClass},
    {UsbGenericHidCallback, (class_handle_t)NULL, &UsbGenericHidClass}
};

usb_device_class_config_list_struct_t UsbDeviceCompositeConfigList = {
    UsbDeviceCompositeClassConfig,
    UsbDeviceCallback,
    USB_COMPOSITE_INTERFACE_COUNT,
};

static usb_status_t UsbDeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    uint16_t *temp16 = (uint16_t *)param;
    uint8_t *temp8 = (uint8_t *)param;

    if (!param && event != kUSB_DeviceEventBusReset && event != kUSB_DeviceEventSetInterface) {
        return error;
    }

    switch (event) {
        case kUSB_DeviceEventBusReset:
            UsbCompositeDevice.attach = 0U;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSetConfiguration:
            UsbCompositeDevice.attach = 1U;
            UsbCompositeDevice.currentConfiguration = *temp8;
            UsbMouseSetConfiguration(UsbCompositeDevice.mouseHandle, *temp8);
            UsbKeyboardSetConfiguration(UsbCompositeDevice.keyboardHandle, *temp8);
            UsbGenericHidSetConfiguration(UsbCompositeDevice.keyboardHandle, *temp8);
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventGetConfiguration:
            *temp8 = UsbCompositeDevice.currentConfiguration;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSetInterface:
            if (UsbCompositeDevice.attach) {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);
                if (interface < USB_COMPOSITE_INTERFACE_COUNT) {
                    UsbCompositeDevice.currentInterfaceAlternateSetting[interface] = alternateSetting;
                    UsbMouseSetInterface(UsbCompositeDevice.mouseHandle, interface, alternateSetting);
                    UsbKeyboardSetInterface(UsbCompositeDevice.keyboardHandle, interface, alternateSetting);
                    UsbGenericHidSetInterface(UsbCompositeDevice.keyboardHandle, interface, alternateSetting);
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetInterface: ;
            uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
            if (interface < USB_COMPOSITE_INTERFACE_COUNT) {
                *temp16 = (*temp16 & 0xFF00U) | UsbCompositeDevice.currentInterfaceAlternateSetting[interface];
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

static void USB_DeviceApplicationInit(void)
{
    uint8_t usbDeviceKhciIrq[] = USB_IRQS;
    uint8_t irqNumber = usbDeviceKhciIrq[CONTROLLER_ID - kUSB_ControllerKhci0];

    SystemCoreClockUpdate();
    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, 48000000U);

    UsbCompositeDevice.attach = 0U;
    USB_DeviceClassInit(CONTROLLER_ID, &UsbDeviceCompositeConfigList, &UsbCompositeDevice.deviceHandle);
    UsbCompositeDevice.keyboardHandle = UsbDeviceCompositeConfigList.config[0].classHandle;
    UsbCompositeDevice.mouseHandle = UsbDeviceCompositeConfigList.config[1].classHandle;
    UsbCompositeDevice.genericHidHandle = UsbDeviceCompositeConfigList.config[2].classHandle;

    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ((IRQn_Type)irqNumber);

    USB_DeviceRun(UsbCompositeDevice.deviceHandle);
}

void main(void)
{
    BOARD_InitPins();
    BOARD_BootClockHSRUN();
    BOARD_InitDebugConsole();

    USB_DeviceApplicationInit();
    while (1U) {
    }
}
