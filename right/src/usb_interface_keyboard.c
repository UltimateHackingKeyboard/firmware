#include "main.h"
#include "action.h"
#include "fsl_port.h"
#include "usb_api.h"
#include "usb_composite_device.h"
#include "test_led.h"
#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_addresses.h"

static usb_device_endpoint_struct_t UsbKeyboardEndpoints[USB_KEYBOARD_ENDPOINT_COUNT] = {{
    USB_KEYBOARD_ENDPOINT_INDEX | (USB_IN << USB_DESCRIPTOR_ENDPOINT_ADDRESS_DIRECTION_SHIFT),
    USB_ENDPOINT_INTERRUPT,
    USB_KEYBOARD_INTERRUPT_IN_PACKET_SIZE,
}};

static usb_device_interface_struct_t UsbKeyboardInterface[] = {{
    USB_INTERFACE_ALTERNATE_SETTING_NONE,
    {USB_KEYBOARD_ENDPOINT_COUNT, UsbKeyboardEndpoints},
    NULL,
}};

static usb_device_interfaces_struct_t UsbKeyboardInterfaces[USB_KEYBOARD_INTERFACE_COUNT] = {{
    USB_CLASS_HID,
    USB_HID_SUBCLASS_BOOT,
    USB_HID_PROTOCOL_KEYBOARD,
    USB_KEYBOARD_INTERFACE_INDEX,
    UsbKeyboardInterface,
    sizeof(UsbKeyboardInterface) / sizeof(usb_device_interfaces_struct_t),
}};

static usb_device_interface_list_t UsbKeyboardInterfaceList[USB_DEVICE_CONFIGURATION_COUNT] = {{
    USB_KEYBOARD_INTERFACE_COUNT,
    UsbKeyboardInterfaces,
}};

usb_device_class_struct_t UsbKeyboardClass = {
    UsbKeyboardInterfaceList,
    kUSB_DeviceClassTypeHid,
    USB_DEVICE_CONFIGURATION_COUNT,
};

volatile static int activeReportIndex=0;
static usb_keyboard_report_t UsbKeyboardReport[2];

void UsbKeyboadTask()
{
    int newReportIndex = 1-activeReportIndex;

    UsbKeyboardReport[newReportIndex].modifiers = 0;
    UsbKeyboardReport[newReportIndex].reserved = 0;

    KeyMatrix_Scan(&KeyMatrix);
    memcpy(CurrentKeyStates[SLOT_ID_RIGHT_KEYBOARD_HALF], KeyMatrix.keyStates, MAX_KEY_COUNT_PER_MODULE);

    uint8_t txData[] = {0};
    bzero(CurrentKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], MAX_KEY_COUNT_PER_MODULE);
    if (I2cWrite(I2C_MAIN_BUS_BASEADDR, I2C_ADDRESS_LEFT_KEYBOARD_HALF, txData, sizeof(txData)) == kStatus_Success) {
        I2cRead(I2C_MAIN_BUS_BASEADDR, I2C_ADDRESS_LEFT_KEYBOARD_HALF, CurrentKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], LEFT_KEYBOARD_HALF_KEY_COUNT);
    }

    bzero(&UsbKeyboardReport[newReportIndex].scancodes, USB_KEYBOARD_MAX_KEYS);
    HandleKeyboardEvents(&UsbKeyboardReport[newReportIndex], &UsbMouseReport);

    activeReportIndex = newReportIndex;
}

static usb_status_t UsbKeyboardAction(void)
{
    return USB_DeviceHidSend(UsbCompositeDevice.keyboardHandle, USB_KEYBOARD_ENDPOINT_INDEX,
                             (uint8_t*)&UsbKeyboardReport[activeReportIndex], USB_KEYBOARD_REPORT_LENGTH);
}

usb_status_t UsbKeyboardCallback(class_handle_t handle, uint32_t event, void *param)
{
    usb_status_t error = kStatus_USB_Error;

    switch (event) {
        case kUSB_DeviceHidEventSendResponse:
            if (UsbCompositeDevice.attach) {
                return UsbKeyboardAction();
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

usb_status_t UsbKeyboardSetConfiguration(class_handle_t handle, uint8_t configuration)
{
    if (USB_COMPOSITE_CONFIGURATION_INDEX == configuration) {
        return UsbKeyboardAction();
    }
    return kStatus_USB_Error;
}

usb_status_t UsbKeyboardSetInterface(class_handle_t handle, uint8_t interface, uint8_t alternateSetting)
{
    if (USB_KEYBOARD_INTERFACE_INDEX == interface) {
        return UsbKeyboardAction();
    }
    return kStatus_USB_Error;
}
