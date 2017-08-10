#include "usb_protocol_handler.h"
#include "system_properties.h"
#include "peripherals/test_led.h"
#include "i2c_addresses.h"
#include "peripherals/led_driver.h"
#include "peripherals/merge_sensor.h"
#include "config_parser/parse_config.h"
#include "config_parser/config_globals.h"
#include "led_pwm.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "wormhole.h"
#include "peripherals/adc.h"
#include "eeprom.h"
#include "keymaps.h"
#include "microseconds/microseconds_pit.c"

// Functions for setting error statuses

void setError(uint8_t error)
{
    GenericHidOutBuffer[0] = error;
}

void setGenericError(void)
{
    setError(UsbResponse_GenericError);
}

void SetResponseByte(uint8_t response)
{
    GenericHidOutBuffer[1] = response;
}

void SetResponseWord(uint16_t response)
{
    *((uint16_t*)(GenericHidOutBuffer+1)) = response;
}

// Per command protocol command handlers

void getSystemProperty(void)
{
    uint8_t propertyId = GenericHidInBuffer[1];

    switch (propertyId) {
        case SystemPropertyId_UsbProtocolVersion:
            SetResponseByte(SYSTEM_PROPERTY_USB_PROTOCOL_VERSION);
            break;
        case SystemPropertyId_BridgeProtocolVersion:
            SetResponseByte(SYSTEM_PROPERTY_BRIDGE_PROTOCOL_VERSION);
            break;
        case SystemPropertyId_DataModelVersion:
            SetResponseByte(SYSTEM_PROPERTY_DATA_MODEL_VERSION);
            break;
        case SystemPropertyId_FirmwareVersion:
            SetResponseByte(SYSTEM_PROPERTY_FIRMWARE_VERSION);
            break;
        case SystemPropertyId_HardwareConfigSize:
            SetResponseWord(HARDWARE_CONFIG_SIZE);
            break;
        case SystemPropertyId_UserConfigSize:
            SetResponseWord(USER_CONFIG_SIZE);
            break;
        default:
            setGenericError();
            break;
    }
}

void reenumerate(void)
{
    Wormhole.magicNumber = WORMHOLE_MAGIC_NUMBER;
    Wormhole.enumerationMode = GenericHidInBuffer[1];
    SCB->AIRCR = 0x5FA<<SCB_AIRCR_VECTKEY_Pos | SCB_AIRCR_SYSRESETREQ_Msk; // Reset the MCU.
    for (;;);
}

void setTestLed(void)
{
    uint8_t ledState = GenericHidInBuffer[1];
    TEST_LED_SET(ledState);
    UhkModuleStates[0].isTestLedOn = ledState;
}

void readMergeSensor(void)
{
    SetResponseByte(MERGE_SENSOR_IS_MERGED);
}

void applyConfig(void)
{
    uint8_t *temp;
    char oldKeymapAbbreviation[3];
    uint8_t oldKeymapAbbreviationLen;

    ParserRunDry = true;
    StagingUserConfigBuffer.offset = 0;
    GenericHidOutBuffer[0] = ParseConfig(&StagingUserConfigBuffer);
    GenericHidOutBuffer[1] = StagingUserConfigBuffer.offset;
    GenericHidOutBuffer[2] = StagingUserConfigBuffer.offset >> 8;
    GenericHidOutBuffer[3] = 0;
    if (GenericHidOutBuffer[0]) {
        return;
    }
    memcpy(oldKeymapAbbreviation, AllKeymaps[CurrentKeymapIndex].abbreviation, 3);
    oldKeymapAbbreviationLen = AllKeymaps[CurrentKeymapIndex].abbreviationLen;
    ParserRunDry = false;
    temp = UserConfigBuffer.buffer;
    UserConfigBuffer.buffer = StagingUserConfigBuffer.buffer;
    StagingUserConfigBuffer.buffer = temp;
    UserConfigBuffer.offset = 0;
    GenericHidOutBuffer[0] = ParseConfig(&UserConfigBuffer);
    GenericHidOutBuffer[1] = UserConfigBuffer.offset;
    GenericHidOutBuffer[2] = UserConfigBuffer.offset >> 8;
    GenericHidOutBuffer[3] = 1;
    if (GenericHidOutBuffer[0]) {
        return;
    }
    for (uint8_t i = 0; i < AllKeymapsCount; i++) {
        if (AllKeymaps[i].abbreviationLen != oldKeymapAbbreviationLen) {
            continue;
        }
        if (memcmp(oldKeymapAbbreviation, AllKeymaps[i].abbreviation, oldKeymapAbbreviationLen)) {
            continue;
        }
        Keymaps_Switch(i);
        return;
    }
    Keymaps_Switch(DefaultKeymapIndex);
}

void setLedPwm(void)
{
    uint8_t brightnessPercent = GenericHidInBuffer[1];
    LedPwm_SetBrightness(brightnessPercent);
    UhkModuleStates[0].ledPwmBrightness = brightnessPercent;
}

void getAdcValue(void)
{
    uint32_t adcValue = ADC_Measure();
    GenericHidOutBuffer[0] = adcValue >> 0;
    GenericHidOutBuffer[1] = adcValue >> 8;
    GenericHidOutBuffer[2] = adcValue >> 16;
    GenericHidOutBuffer[3] = adcValue >> 24;
}

void launchEepromTransfer(void)
{
    eeprom_transfer_t transferType = GenericHidInBuffer[1];
    EEPROM_LaunchTransfer(transferType);
}

void readConfiguration(bool isHardware)
{
    uint8_t length = GenericHidInBuffer[1];
    uint16_t offset = *((uint16_t*)(GenericHidInBuffer+2));

    if (length > USB_GENERIC_HID_OUT_BUFFER_LENGTH-1) {
        setError(ConfigTransferResponse_LengthTooLarge);
        return;
    }

    uint8_t *buffer = isHardware ? HardwareConfigBuffer.buffer : UserConfigBuffer.buffer;
    uint16_t bufferLength = isHardware ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;

    if (offset + length > bufferLength) {
        setError(ConfigTransferResponse_BufferOutOfBounds);
        return;
    }

    memcpy(GenericHidOutBuffer+1, buffer+offset, length);
}

void writeConfiguration(bool isHardware)
{
    uint8_t length = GenericHidInBuffer[1];
    uint16_t offset = *((uint16_t*)(GenericHidInBuffer+1+1));

    if (length > USB_GENERIC_HID_OUT_BUFFER_LENGTH-1-1-2) {
        setError(ConfigTransferResponse_LengthTooLarge);
        return;
    }

    uint8_t *buffer = isHardware ? HardwareConfigBuffer.buffer : StagingUserConfigBuffer.buffer;
    uint16_t bufferLength = isHardware ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;

    if (offset + length > bufferLength) {
        setError(ConfigTransferResponse_BufferOutOfBounds);
        return;
    }

    memcpy(buffer+offset, GenericHidInBuffer+1+1+2, length);
}

void getKeyboardState(void)
{
    GenericHidOutBuffer[1] = IsEepromBusy;
}

void getDebugInfo(void)
{
    GenericHidOutBuffer[1] = I2C_Watchdog >> 0;
    GenericHidOutBuffer[2] = I2C_Watchdog >> 8;
    GenericHidOutBuffer[3] = I2C_Watchdog >> 16;
    GenericHidOutBuffer[4] = I2C_Watchdog >> 24;

    uint64_t ticks = microseconds_get_ticks();
    uint32_t microseconds = microseconds_convert_to_microseconds(ticks);
    uint32_t milliseconds = microseconds/1000;

    GenericHidOutBuffer[1] = (ticks >> 0) & 0xff;;
    GenericHidOutBuffer[2] = (ticks >> 8) & 0xff;
    GenericHidOutBuffer[3] = (ticks >> 16) & 0xff;
    GenericHidOutBuffer[4] = (ticks >> 24) & 0xff;
    GenericHidOutBuffer[5] = (ticks >> 32) & 0xff;;
    GenericHidOutBuffer[6] = (ticks >> 40) & 0xff;
    GenericHidOutBuffer[7] = (ticks >> 48) & 0xff;
    GenericHidOutBuffer[8] = (ticks >> 56) & 0xff;
}

// The main protocol handler function

void usbProtocolHandler(void)
{
    bzero(GenericHidOutBuffer, USB_GENERIC_HID_OUT_BUFFER_LENGTH);
    uint8_t command = GenericHidInBuffer[0];
    switch (command) {
        case UsbCommand_GetSystemProperty:
            getSystemProperty();
            break;
        case UsbCommand_Reenumerate:
            reenumerate();
            break;
        case UsbCommand_SetTestLed:
            setTestLed();
            break;
        case UsbCommand_WriteLedDriver:
            break;
        case UsbCommand_ReadMergeSensor:
            readMergeSensor();
            break;
        case UsbCommand_WriteUserConfiguration:
            writeConfiguration(false);
            break;
        case UsbCommand_ApplyConfig:
            applyConfig();
            break;
        case UsbCommand_SetLedPwm:
            setLedPwm();
            break;
        case UsbCommand_GetAdcValue:
            getAdcValue();
            break;
        case UsbCommand_LaunchEepromTransfer:
            launchEepromTransfer();
            break;
        case UsbCommand_ReadHardwareConfiguration:
            readConfiguration(true);
            break;
        case UsbCommand_WriteHardwareConfiguration:
            writeConfiguration(true);
            break;
        case UsbCommand_ReadUserConfiguration:
            readConfiguration(false);
            break;
        case UsbCommand_GetKeyboardState:
            getKeyboardState();
            break;
        case UsbCommand_GetDebugInfo:
            getDebugInfo();
            break;
        default:
            break;
    }
}
