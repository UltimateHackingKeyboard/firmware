#include "fsl_port.h"
#include "usb_api.h"
#include "usb_composite_device.h"
#include "test_led.h"
#include "key_matrix.h"

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

#define KEYBOARD_MATRIX_COLS_NUM 7
#define KEYBOARD_MATRIX_ROWS_NUM 5

key_matrix_t keyMatrix = {
    .colNum = KEYBOARD_MATRIX_COLS_NUM,
    .rowNum = KEYBOARD_MATRIX_ROWS_NUM,
    .cols = (key_matrix_pin_t[]){
        {PORTA, GPIOA, kCLOCK_PortA, 5},
        {PORTB, GPIOB, kCLOCK_PortB, 3},
        {PORTB, GPIOB, kCLOCK_PortB, 16},
        {PORTB, GPIOB, kCLOCK_PortB, 17},
        {PORTB, GPIOB, kCLOCK_PortB, 18},
        {PORTA, GPIOA, kCLOCK_PortA, 1},
        {PORTB, GPIOB, kCLOCK_PortB, 0}
        },
    .rows = (key_matrix_pin_t[]){
        {PORTA, GPIOA, kCLOCK_PortA, 12},
        {PORTA, GPIOA, kCLOCK_PortA, 13},
        {PORTC, GPIOC, kCLOCK_PortC, 0},
        {PORTB, GPIOB, kCLOCK_PortB, 19},
        {PORTD, GPIOD, kCLOCK_PortD, 6}
    }
};

uint8_t keystates[100];

static usb_status_t UsbKeyboardAction(void)
{
    UsbKeyboardReport.modifiers = 0;
    UsbKeyboardReport.reserved = 0;

    KeyMatrix_Init(&keyMatrix);
    //KeyMatrix_Scan(&keyMatrix);

    for (uint8_t scancode_idx=0; scancode_idx<USB_KEYBOARD_MAX_KEYS; scancode_idx++) {
        UsbKeyboardReport.scancodes[scancode_idx] = 0;
    }

    uint8_t scancode_idx = 0;
    for (uint8_t col=0; col<KEYBOARD_MATRIX_COLS_NUM; col++) {
        GPIO_WritePinOutput(keyMatrix.cols[col].gpio, keyMatrix.cols[col].pin, 1);
        for (uint8_t row=0; row<KEYBOARD_MATRIX_ROWS_NUM; row++) {
            if (GPIO_ReadPinInput(keyMatrix.rows[row].gpio, keyMatrix.rows[row].pin)) {
                GPIO_SetPinsOutput(TEST_LED_GPIO, 1 << TEST_LED_GPIO_PIN);
                UsbKeyboardReport.scancodes[scancode_idx++] = HID_KEYBOARD_SC_A + row*KEYBOARD_MATRIX_COLS_NUM + col;
            }
        }
        GPIO_WritePinOutput(keyMatrix.cols[col].gpio, keyMatrix.cols[col].pin, 0);
        for (volatile uint32_t i=0; i<100; i++);
    }

/*
    uint8_t keyId = 0;
    for (uint8_t col=0; col<keyMatrix.colNum; col++) {
        GPIO_WritePinOutput(keyMatrix.cols[col].gpio, keyMatrix.cols[col].pin, 1);
        for (uint8_t row=0; row<keyMatrix.rowNum; row++) {
            keystates[row+col*keyMatrix.rowNum] = GPIO_ReadPinInput(keyMatrix.rows[row].gpio, keyMatrix.rows[row].pin);
        }
        GPIO_WritePinOutput(keyMatrix.cols[col].gpio, keyMatrix.cols[col].pin, 0);
    }

    for (uint8_t keyId=0; keyId<KEYBOARD_MATRIX_COLS_NUM*KEYBOARD_MATRIX_ROWS_NUM; keyId++) {
        if (keystates[keyId] && scancode_idx<USB_KEYBOARD_MAX_KEYS) {
            UsbKeyboardReport.scancodes[scancode_idx++] = keyId + HID_KEYBOARD_SC_A;
        }
    }
*/
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
