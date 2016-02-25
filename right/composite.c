#include "usb_device_config.h"
#include "usb.h"
#include "usb_device.h"
#include "include/usb/usb_device_class.h"
#include "include/usb/usb_device_hid.h"
#include "include/usb/usb_device_ch9.h"
#include "usb_device_descriptor.h"
#include "composite.h"
#include "hid_keyboard.h"
#include "hid_mouse.h"
#include "fsl_device_registers.h"
#include "include/board/clock_config.h"
#include "include/board/board.h"
#include "fsl_debug_console.h"
#include <stdio.h>
#include <stdlib.h>
#include "fsl_common.h"
#include "include/board/pin_mux.h"
#include "usb_keyboard_descriptors.h"
#include "usb_mouse_descriptors.h"

static usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param);
static usb_device_composite_struct_t UsbDeviceComposite;

usb_device_class_config_struct_t UsbDeviceCompositeClassConfig[USB_COMPOSITE_INTERFACE_COUNT] = {
    {USB_DeviceHidKeyboardCallback, (class_handle_t)NULL, &UsbKeyboardClass},
    {USB_DeviceHidMouseCallback,    (class_handle_t)NULL, &UsbDeviceMouseClass}
};

usb_device_class_config_list_struct_t g_UsbDeviceCompositeConfigList = {
    UsbDeviceCompositeClassConfig,
    USB_DeviceCallback,
    USB_COMPOSITE_INTERFACE_COUNT,
};

static usb_status_t USB_DeviceCallback(usb_device_handle handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;
    uint16_t *temp16 = (uint16_t *)param;
    uint8_t *temp8 = (uint8_t *)param;

    if (!param && event != kUSB_DeviceEventBusReset && event != kUSB_DeviceEventSetInterface) {
        return error;
    }

    switch (event) {
        case kUSB_DeviceEventBusReset:
            UsbDeviceComposite.attach = 0U;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSetConfiguration:
            UsbDeviceComposite.attach = 1U;
            UsbDeviceComposite.currentConfiguration = *temp8;
            USB_DeviceHidMouseSetConfigure(UsbDeviceComposite.hidMouseHandle, *temp8);
            USB_DeviceHidKeyboardSetConfigure(UsbDeviceComposite.hidKeyboardHandle, *temp8);
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventGetConfiguration:
            *temp8 = UsbDeviceComposite.currentConfiguration;
            error = kStatus_USB_Success;
            break;
        case kUSB_DeviceEventSetInterface:
            if (UsbDeviceComposite.attach) {
                uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
                uint8_t alternateSetting = (uint8_t)(*temp16 & 0x00FFU);
                if (interface < USB_COMPOSITE_INTERFACE_COUNT) {
                    UsbDeviceComposite.currentInterfaceAlternateSetting[interface] = alternateSetting;
                    USB_DeviceHidMouseSetInterface(UsbDeviceComposite.hidMouseHandle, interface, alternateSetting);
                    USB_DeviceHidKeyboardSetInterface(UsbDeviceComposite.hidKeyboardHandle, interface,
                                                      alternateSetting);
                    error = kStatus_USB_Success;
                }
            }
            break;
        case kUSB_DeviceEventGetInterface: ;
            uint8_t interface = (uint8_t)((*temp16 & 0xFF00U) >> 0x08U);
            if (interface < USB_COMPOSITE_INTERFACE_COUNT) {
                *temp16 = (*temp16 & 0xFF00U) | UsbDeviceComposite.currentInterfaceAlternateSetting[interface];
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
    USB_DeviceKhciIsrFunction(UsbDeviceComposite.deviceHandle);
}

static void USB_DeviceApplicationInit(void)
{
    uint8_t usbDeviceKhciIrq[] = USB_IRQS;
    uint8_t irqNumber = usbDeviceKhciIrq[CONTROLLER_ID - kUSB_ControllerKhci0];

    SystemCoreClockUpdate();

    CLOCK_EnableUsbfs0Clock(kCLOCK_UsbSrcIrc48M, 48000000U);

    // Set composite device to default state.
    UsbDeviceComposite.speed = USB_SPEED_FULL;
    UsbDeviceComposite.attach = 0U;
    UsbDeviceComposite.hidMouseHandle = (class_handle_t)NULL;
    UsbDeviceComposite.hidKeyboardHandle = (class_handle_t)NULL;
    UsbDeviceComposite.deviceHandle = NULL;

    usb_status_t deviceStatus = USB_DeviceClassInit(
        CONTROLLER_ID, &g_UsbDeviceCompositeConfigList, &UsbDeviceComposite.deviceHandle);

    if (kStatus_USB_Success != deviceStatus) {
        usb_echo("USB device composite demo init failed\r\n");
        return;
    } else {
        usb_echo("USB device composite demo\r\n");
        UsbDeviceComposite.hidKeyboardHandle = g_UsbDeviceCompositeConfigList.config[0].classHandle;
        UsbDeviceComposite.hidMouseHandle = g_UsbDeviceCompositeConfigList.config[1].classHandle;
        USB_DeviceHidKeyboardInit(&UsbDeviceComposite);
        USB_DeviceHidMouseInit(&UsbDeviceComposite);
    }

    // Install ISR, set priority, and enable IRQ.
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_DEVICE_INTERRUPT_PRIORITY);
    NVIC_EnableIRQ((IRQn_Type)irqNumber);

    // Start the device function.
    USB_DeviceRun(UsbDeviceComposite.deviceHandle);
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
