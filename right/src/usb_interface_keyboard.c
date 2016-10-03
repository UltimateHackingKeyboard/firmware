#include "fsl_port.h"
#include "include/board/board.h"
#include "usb_api.h"
#include "usb_composite_device.h"
#include "test_led.h"

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

static usb_keyboard_report_t UsbKeyboardReport;

typedef struct {
    PORT_Type *port;
    GPIO_Type *gpio;
    uint32_t pin;
} pin_t;

#define KEYBOARD_MATRIX_COLS_NUM 7
#define KEYBOARD_MATRIX_ROWS_NUM 5

pin_t keyboardMatrixCols[] = {
    {PORTA, GPIOA, 5},
    {PORTB, GPIOB, 3},
    {PORTB, GPIOB, 16},
    {PORTB, GPIOB, 17},
    {PORTB, GPIOB, 18},
    {PORTA, GPIOA, 1},
    {PORTB, GPIOB, 0},
};

pin_t keyboardMatrixRows[] = {
    {PORTA, GPIOA, 12},
    {PORTA, GPIOA, 13},
    {PORTC, GPIOC, 0},
    {PORTB, GPIOB, 19},
    {PORTD, GPIOD, 6},
};

static usb_status_t UsbKeyboardAction(void)
{
    uint8_t scancode_i = 0;
    UsbKeyboardReport.modifiers = 0;
    UsbKeyboardReport.reserved = 0;

    for (uint8_t scancode_idx=0; scancode_idx<USB_KEYBOARD_MAX_KEYS; scancode_idx++) {
        UsbKeyboardReport.scancodes[scancode_idx] = 0;
    }

    for (uint8_t col=0; col<KEYBOARD_MATRIX_COLS_NUM; col++) {
        PORT_SetPinConfig(keyboardMatrixCols[col].port, keyboardMatrixCols[col].pin, &(port_pin_config_t){.pullSelect=kPORT_PullDisable, .mux=kPORT_MuxAsGpio});
        GPIO_PinInit(keyboardMatrixCols[col].gpio, keyboardMatrixCols[col].pin, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalOutput, .outputLogic=1});
        GPIO_WritePinOutput(keyboardMatrixCols[col].gpio, keyboardMatrixCols[col].pin, 1);

        for (uint8_t row=0; row<KEYBOARD_MATRIX_ROWS_NUM; row++) {
            PORT_SetPinConfig(keyboardMatrixRows[row].port, keyboardMatrixRows[row].pin, &(port_pin_config_t){.pullSelect=kPORT_PullDown, .mux=kPORT_MuxAsGpio});
            GPIO_PinInit(keyboardMatrixRows[row].gpio, keyboardMatrixRows[row].pin, &(gpio_pin_config_t){.pinDirection=kGPIO_DigitalInput});

            if (GPIO_ReadPinInput(keyboardMatrixRows[row].gpio, keyboardMatrixRows[row].pin)) {
                GPIO_SetPinsOutput(TEST_LED_GPIO, 1 << TEST_LED_GPIO_PIN);
                UsbKeyboardReport.scancodes[scancode_i] = HID_KEYBOARD_SC_A + row*KEYBOARD_MATRIX_COLS_NUM + col;
            }
        }
        GPIO_WritePinOutput(keyboardMatrixCols[col].gpio, keyboardMatrixCols[col].pin, 0);
    }

    return USB_DeviceHidSend(UsbCompositeDevice.keyboardHandle, USB_KEYBOARD_ENDPOINT_INDEX,
                             (uint8_t*)&UsbKeyboardReport, USB_KEYBOARD_REPORT_LENGTH);
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
