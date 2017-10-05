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
#include "i2c_watchdog.h"
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
    UhkModuleVars[0].isTestLedOn = ledState;
}

void readMergeSensor(void)
{
    SetResponseByte(MERGE_SENSOR_IS_MERGED);
}

void ApplyConfig(void)
{
    // Validate the staging configuration.

    ParserRunDry = true;
    StagingUserConfigBuffer.offset = 0;
    GenericHidOutBuffer[0] = ParseConfig(&StagingUserConfigBuffer);
    GenericHidOutBuffer[1] = StagingUserConfigBuffer.offset;
    GenericHidOutBuffer[2] = StagingUserConfigBuffer.offset >> 8;
    GenericHidOutBuffer[3] = 0;

    if (GenericHidOutBuffer[0] != UsbResponse_Success) {
        return;
    }

    // Make the staging configuration the current one.

    char oldKeymapAbbreviation[KEYMAP_ABBREVIATION_LENGTH];
    uint8_t oldKeymapAbbreviationLen;
    memcpy(oldKeymapAbbreviation, AllKeymaps[CurrentKeymapIndex].abbreviation, KEYMAP_ABBREVIATION_LENGTH);
    oldKeymapAbbreviationLen = AllKeymaps[CurrentKeymapIndex].abbreviationLen;

    uint8_t *temp;
    temp = ValidatedUserConfigBuffer.buffer;
    ValidatedUserConfigBuffer.buffer = StagingUserConfigBuffer.buffer;
    StagingUserConfigBuffer.buffer = temp;

    ParserRunDry = false;
    ValidatedUserConfigBuffer.offset = 0;
    GenericHidOutBuffer[0] = ParseConfig(&ValidatedUserConfigBuffer);
    GenericHidOutBuffer[1] = ValidatedUserConfigBuffer.offset;
    GenericHidOutBuffer[2] = ValidatedUserConfigBuffer.offset >> 8;
    GenericHidOutBuffer[3] = 1;

    if (GenericHidOutBuffer[0] != UsbResponse_Success) {
        return;
    }

    // Switch to the keymap of the updated configuration of the same name or the default keymap.

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
    UhkModuleVars[0].ledPwmBrightness = brightnessPercent;
}

void getAdcValue(void)
{
    uint32_t adcValue = ADC_Measure();
    GenericHidOutBuffer[0] = adcValue >> 0;
    GenericHidOutBuffer[1] = adcValue >> 8;
    GenericHidOutBuffer[2] = adcValue >> 16;
    GenericHidOutBuffer[3] = adcValue >> 24;
}

void legacyLaunchEepromTransfer(void)
{
    uint8_t legacyEepromTransferId = GenericHidInBuffer[1];
    switch (legacyEepromTransferId) {
    case 0:
        EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_HardwareConfig, NULL);
        break;
    case 1:
        EEPROM_LaunchTransfer(EepromOperation_Write, ConfigBufferId_HardwareConfig, NULL);
        break;
    case 2:
        EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_ValidatedUserConfig, NULL);
        break;
    case 3:
        EEPROM_LaunchTransfer(EepromOperation_Write, ConfigBufferId_ValidatedUserConfig, NULL);
        break;
    }
}

void readConfiguration(bool isHardware)
{
    uint8_t length = GenericHidInBuffer[1];
    uint16_t offset = *((uint16_t*)(GenericHidInBuffer+2));

    if (length > USB_GENERIC_HID_OUT_BUFFER_LENGTH-1) {
        setError(ConfigTransferResponse_LengthTooLarge);
        return;
    }

    uint8_t *buffer = isHardware ? HardwareConfigBuffer.buffer : ValidatedUserConfigBuffer.buffer;
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

    GenericHidOutBuffer[5] = I2cSchedulerCounter >> 0;
    GenericHidOutBuffer[6] = I2cSchedulerCounter >> 8;
    GenericHidOutBuffer[7] = I2cSchedulerCounter >> 16;
    GenericHidOutBuffer[8] = I2cSchedulerCounter >> 24;

    GenericHidOutBuffer[9] = I2cWatchdog_OuterCounter >> 0;
    GenericHidOutBuffer[10] = I2cWatchdog_OuterCounter >> 8;
    GenericHidOutBuffer[11] = I2cWatchdog_OuterCounter >> 16;
    GenericHidOutBuffer[12] = I2cWatchdog_OuterCounter >> 24;

    GenericHidOutBuffer[13] = I2cWatchdog_InnerCounter >> 0;
    GenericHidOutBuffer[14] = I2cWatchdog_InnerCounter >> 8;
    GenericHidOutBuffer[15] = I2cWatchdog_InnerCounter >> 16;
    GenericHidOutBuffer[16] = I2cWatchdog_InnerCounter >> 24;
/*
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
*/}

// The main protocol handler function

void UsbProtocolHandler(void)
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
            ApplyConfig();
            break;
        case UsbCommand_SetLedPwm:
            setLedPwm();
            break;
        case UsbCommand_GetAdcValue:
            getAdcValue();
            break;
        case UsbCommand_LaunchEepromTransfer:
            legacyLaunchEepromTransfer();
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
