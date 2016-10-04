#include "usb_protocol_handler.h"
#include "system_properties.h"
#include "test_led.h"
#include "i2c_addresses.h"
#include "led_driver.h"

void SetError(uint8_t error);
void SetGenericError();
void UsbProtocolHandler();
void GetSystemProperty();
void JumpToBootloader();
void GetSetTestLed();
void WriteLedDriver();
void ReadLedDriver();

// Functions for setting error statuses

void SetError(uint8_t error) {
    GenericHidOutBuffer[0] = error;
}

void SetGenericError()
{
    SetError(PROTOCOL_RESPONSE_GENERIC_ERROR);
}

// Set a single byte as the response.
void SetResponseByte(uint8_t response)
{
    GenericHidOutBuffer[1] = response;
}

// The main protocol handler function

void UsbProtocolHandler()
{
    uint8_t command = GenericHidInBuffer[0];
    switch (command) {
        case USB_COMMAND_GET_SYSTEM_PROPERTY:
            GetSystemProperty();
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

// Per command protocol command handlers

void GetSystemProperty() {
    uint8_t propertyId = GenericHidInBuffer[1];

    switch (propertyId) {
        case SYSTEM_PROPERTY_USB_PROTOCOL_VERSION_ID:
            SetResponseByte(SYSTEM_PROPERTY_USB_PROTOCOL_VERSION);
            break;
        case SYSTEM_PROPERTY_BRIDGE_PROTOCOL_VERSION_ID:
            SetResponseByte(SYSTEM_PROPERTY_BRIDGE_PROTOCOL_VERSION);
            break;
        case SYSTEM_PROPERTY_DATA_MODEL_VERSION_ID:
            SetResponseByte(SYSTEM_PROPERTY_DATA_MODEL_VERSION);
            break;
        case SYSTEM_PROPERTY_FIRMWARE_VERSION_ID:
            SetResponseByte(SYSTEM_PROPERTY_FIRMWARE_VERSION);
            break;
        default:
            SetGenericError();
            break;
    }
}

void JumpToBootloader() {
}

void GetSetTestLed()
{
    uint8_t arg = GenericHidInBuffer[1];
    switch (arg) {
        case 0:
            TEST_LED_ON();
            break;
        case 1:
            TEST_LED_OFF();
            break;
    }
}

void WriteLedDriver()
{
    uint8_t i2cAddress = GenericHidInBuffer[1];
    uint8_t i2cPayloadSize = GenericHidInBuffer[2];

    if (!IS_I2C_LED_DRIVER_ADDRESS(i2cAddress)) {
        SetError(WRITE_LED_DRIVER_RESPONSE_INVALID_ADDRESS);
        return;
    }

    if (i2cPayloadSize > USB_GENERIC_HID_OUT_BUFFER_LENGTH-3) {
        SetError(WRITE_LED_DRIVER_RESPONSE_INVALID_PAYLOAD_SIZE);
        return;
    }

    LedDriver_WriteBuffer(i2cAddress, GenericHidInBuffer+3, i2cPayloadSize);
}

void ReadLedDriver()
{
}
