#include "usb_protocol_handler.h"
#include "test_led.h"

void UsbProtocolHandler()
{
    uint8_t command = GenericHidInBuffer[0];
    uint8_t arg = GenericHidInBuffer[1];

    switch (command) {
        case USB_COMMAND_JUMP_TO_BOOTLOADER:
            break;
        case USB_COMMAND_TEST_LED:
            switch (arg) {
                case 0:
                    TEST_RED_ON();
                    break;
                case 1:
                    TEST_RED_OFF();
                    break;
            }
        case USB_COMMAND_LED_DRIVER:
            break;
        default:
            break;
    }
}
