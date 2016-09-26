#include "usb_protocol_handler.h"
#include "test_led.h"

void GetProtocolVersion();
void JumpToBootloader();
void GetSetTestLed();
void WriteLedDriver();
void ReadLedDriver();

void UsbProtocolHandler()
{
    uint8_t command = GenericHidInBuffer[0];
    switch (command) {
        case USB_COMMAND_GET_PROTOCOL_VERSION:
            GetProtocolVersion();
            break;
        case USB_COMMAND_JUMP_TO_BOOTLOADER:
            JumpToBootloader();
            break;
        case USB_COMMAND_TEST_LED:
            GetSetTestLed();
            break;
        case USB_COMMAND_WRITE_LED_DRIVER:
            WriteLedDriver();
            break;
        case USB_COMMAND_READ_LED_DRIVER:
            ReadLedDriver();
            break;
        default:
            break;
    }
}

void GetProtocolVersion() {
    GenericHidOutBuffer[1] = 1;
}

void JumpToBootloader() {
}

void GetSetTestLed()
{
    uint8_t arg = GenericHidInBuffer[1];
    switch (arg) {
        case 0:
            TEST_RED_ON();
            break;
        case 1:
            TEST_RED_OFF();
            break;
    }
}

void WriteLedDriver()
{
}

void ReadLedDriver()
{
}
