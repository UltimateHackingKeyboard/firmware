#include "usb_protocol_handler.h"
#include "system_properties.h"
#include "test_led.h"
#include "i2c_addresses.h"
#include "led_driver.h"
#include "merge_sensor.h"
#include "deserialize.h"
#include "config_buffer.h"
#include "led_pwm.h"
#include "bridge_protocol_scheduler.h"

void setError(uint8_t error);
void setGenericError();
void usbProtocolHandler();
void getSystemProperty();
void jumpToBootloader();
void getSetTestLed();
void writeLedDriver();
void readLedDriver();
void writeEeprom();
void readEeprom();
void readMergeSensor();
void uploadConfig();
void applyConfig();
void setLedPwm();

// Functions for setting error statuses

void setError(uint8_t error) {
    GenericHidOutBuffer[0] = error;
}

void setGenericError()
{
    setError(PROTOCOL_RESPONSE_GENERIC_ERROR);
}

// Set a single byte as the response.
void SetResponseByte(uint8_t response)
{
    GenericHidOutBuffer[1] = response;
}

// The main protocol handler function

void usbProtocolHandler()
{
    bzero(GenericHidOutBuffer, USB_GENERIC_HID_OUT_BUFFER_LENGTH);
    uint8_t command = GenericHidInBuffer[0];
    switch (command) {
        case USB_COMMAND_GET_SYSTEM_PROPERTY:
            getSystemProperty();
            break;
        case USB_COMMAND_JUMP_TO_BOOTLOADER:
            jumpToBootloader();
            break;
        case USB_COMMAND_TEST_LED:
            getSetTestLed();
            break;
        case USB_COMMAND_WRITE_LED_DRIVER:
            //writeLedDriver();
            break;
        case USB_COMMAND_READ_LED_DRIVER:
            //readLedDriver();
            break;
        case USB_COMMAND_WRITE_EEPROM:
            writeEeprom();
            break;
        case USB_COMMAND_READ_EEPROM:
            readEeprom();
            break;
        case USB_COMMAND_READ_MERGE_SENSOR:
            readMergeSensor();
            break;
        case USB_COMMAND_UPLOAD_CONFIG:
            uploadConfig();
            break;
        case USB_COMMAND_APPLY_CONFIG:
            applyConfig();
            break;
        case USB_COMMAND_SET_LED_PWM:
            setLedPwm();
            break;
        default:
            break;
    }
}

// Per command protocol command handlers

void getSystemProperty() {
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
            setGenericError();
            break;
    }
}

void jumpToBootloader() {
    // We should reset the device here
    SCB->AIRCR = (0x5FA<<SCB_AIRCR_VECTKEY_Pos)|SCB_AIRCR_SYSRESETREQ_Msk;
    //SCB->AIRCR = 0x05fA0002; // If the masked version doesn't work, this should also reset the core.
    for(;;);
}

void getSetTestLed()
{
    uint8_t ledState = GenericHidInBuffer[1];
//    uint8_t data[] = {1, ledState};
//    I2cWrite(I2C_MAIN_BUS_BASEADDR, I2C_ADDRESS_LEFT_KEYBOARD_HALF, data, sizeof(data));

    switch (ledState) {
        case 0:
            TEST_LED_ON();
            break;
        case 1:
            TEST_LED_OFF();
            break;
    }
    SetLeds(ledState ? 0xff : 0);
}

void writeLedDriver()
{
    uint8_t i2cAddress = GenericHidInBuffer[1];
    uint8_t i2cPayloadSize = GenericHidInBuffer[2];

    if (!IS_I2C_LED_DRIVER_ADDRESS(i2cAddress)) {
        setError(WRITE_LED_DRIVER_RESPONSE_INVALID_ADDRESS);
        return;
    }

    if (i2cPayloadSize > USB_GENERIC_HID_OUT_BUFFER_LENGTH-3) {
        setError(WRITE_LED_DRIVER_RESPONSE_INVALID_PAYLOAD_SIZE);
        return;
    }

    I2cWrite(I2C_MAIN_BUS_BASEADDR, i2cAddress, GenericHidInBuffer+3, i2cPayloadSize);
}

void readLedDriver()
{

}

void writeEeprom()
{
    uint8_t i2cPayloadSize = GenericHidInBuffer[1];

    if (i2cPayloadSize > USB_GENERIC_HID_OUT_BUFFER_LENGTH-2) {
        setError(WRITE_EEPROM_RESPONSE_INVALID_PAYLOAD_SIZE);
        return;
    }

    I2cWrite(I2C_EEPROM_BUS_BASEADDR, I2C_ADDRESS_EEPROM, GenericHidInBuffer+2, i2cPayloadSize);
}

void readEeprom()
{
    uint8_t i2cPayloadSize = GenericHidInBuffer[1];

    if (i2cPayloadSize > USB_GENERIC_HID_OUT_BUFFER_LENGTH-1) {
        setError(WRITE_EEPROM_RESPONSE_INVALID_PAYLOAD_SIZE);
        return;
    }

    I2cWrite(I2C_EEPROM_BUS_BASEADDR, I2C_ADDRESS_EEPROM, GenericHidInBuffer+2, 2);
    I2cRead(I2C_EEPROM_BUS_BASEADDR, I2C_ADDRESS_EEPROM, GenericHidOutBuffer+1, i2cPayloadSize);

    GenericHidOutBuffer[0] = PROTOCOL_RESPONSE_SUCCESS;
}

void readMergeSensor()
{
    SetResponseByte(MERGE_SENSOR_IS_MERGED);
}

void uploadConfig()
{
    uint8_t byteCount = GenericHidInBuffer[1];
    uint16_t memoryOffset = *((uint16_t*)(GenericHidInBuffer+2));

    if (byteCount > USB_GENERIC_HID_OUT_BUFFER_LENGTH-4) {
        setError(UPLOAD_CONFIG_INVALID_PAYLOAD_SIZE);
        return;
    }

    memcpy(ConfigBuffer+memoryOffset, GenericHidInBuffer+4, byteCount);
}

void applyConfig()
{
    deserialize_Layer(ConfigBuffer, 0);
}

void setLedPwm()
{
#if UHK_PCB_MAJOR_VERSION >= 7
    uint8_t isRightKeyboardHalf = GenericHidInBuffer[1];
    uint8_t brightnessPercent = GenericHidInBuffer[2];
    if (isRightKeyboardHalf) {
        LedPwm_SetBrightness(brightnessPercent);
    } else {
        uint8_t data[] = {2, brightnessPercent};
        I2cWrite(I2C_MAIN_BUS_BASEADDR, I2C_ADDRESS_LEFT_KEYBOARD_HALF, data, sizeof(data));
    }
#endif
}
