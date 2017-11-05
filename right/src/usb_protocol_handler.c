#include "usb_protocol_handler.h"
#include "peripherals/test_led.h"
#include "i2c_addresses.h"
#include "peripherals/led_driver.h"
#include "peripherals/merge_sensor.h"
#include "config_parser/parse_config.h"
#include "config_parser/config_globals.h"
#include "led_pwm.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_drivers/kboot_driver.h"
#include "bootloader/wormhole.h"
#include "peripherals/adc.h"
#include "eeprom.h"
#include "keymap.h"
#include "microseconds/microseconds_pit.c"
#include "i2c_watchdog.h"
#include "usb_commands/usb_command_apply_config.h"
#include "usb_commands/usb_command_read_config.h"
#include "usb_commands/usb_command_write_config.h"
#include "usb_commands/usb_command_get_property.h"
#include "usb_commands/usb_command_jump_to_slave_bootloader.h"
#include "usb_commands/usb_command_send_kboot_command.h"
#include "usb_commands/usb_command_launch_eeprom_transfer_legacy.h"
#include "usb_commands/usb_command_get_keyboard_state.h"
#include "usb_commands/usb_command_get_debug_info.h"
#include "usb_commands/usb_command_reenumerate.h"
#include "usb_commands/usb_command_set_test_led.h"

uint8_t UsbDebugInfo[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

// Functions for setting error statuses

void SetUsbError(uint8_t error)
{
    GenericHidOutBuffer[0] = error;
}

void SetUsbResponseByte(uint8_t response)
{
    GenericHidOutBuffer[1] = response;
}

void SetUsbResponseWord(uint16_t response)
{
    *((uint16_t*)(GenericHidOutBuffer+1)) = response;
}

// Per command protocol command handlers

void setLedPwm(void)
{
    uint8_t brightnessPercent = GenericHidInBuffer[1];
    LedPwm_SetBrightness(brightnessPercent);
    UhkModuleStates[UhkModuleDriverId_LeftKeyboardHalf].sourceVars.ledPwmBrightness = brightnessPercent;
}

void getAdcValue(void)
{
    *(uint32_t*)(GenericHidOutBuffer+1) = ADC_Measure();
}

// The main protocol handler function

void UsbProtocolHandler(void)
{
    bzero(GenericHidOutBuffer, USB_GENERIC_HID_OUT_BUFFER_LENGTH);
    uint8_t command = GenericHidInBuffer[0];
    switch (command) {
        case UsbCommandId_GetProperty:
            UsbCommand_GetProperty();
            break;
        case UsbCommandId_Reenumerate:
            UsbCommand_Reenumerate();
            break;
        case UsbCommandId_SetTestLed:
            UsbCommand_SetTestLed();
            break;
        case UsbCommandId_WriteLedDriver:
            break;
        case UsbCommandId_WriteUserConfiguration:
            UsbCommand_WriteConfig(false);
            break;
        case UsbCommandId_ApplyConfig:
            UsbCommand_ApplyConfig();
            break;
        case UsbCommandId_SetLedPwm:
            setLedPwm();
            break;
        case UsbCommandId_GetAdcValue:
            getAdcValue();
            break;
        case UsbCommandId_LaunchEepromTransferLegacy:
            UsbCommand_LaunchEepromTransferLegacy();
            break;
        case UsbCommandId_ReadHardwareConfiguration:
            UsbCommand_ReadConfig(true);
            break;
        case UsbCommandId_WriteHardwareConfiguration:
            UsbCommand_WriteConfig(true);
            break;
        case UsbCommandId_ReadUserConfiguration:
            UsbCommand_ReadConfig(false);
            break;
        case UsbCommandId_GetKeyboardState:
            UsbCommand_GetKeyboardState();
            break;
        case UsbCommandId_GetDebugInfo:
            UsbCommand_GetDebugInfo();
            break;
        case UsbCommandId_JumpToSlaveBootloader:
            UsbCommand_JumpToSlaveBootloader();
            break;
        case UsbCommandId_SendKbootCommand:
            UsbCommand_SendKbootCommand();
            break;
        default:
            break;
    }
}
