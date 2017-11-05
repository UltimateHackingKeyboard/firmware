#include "usb_protocol_handler.h"
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
#include "usb_commands/usb_command_get_adc_value.h"
#include "usb_commands/usb_command_set_led_pwm_brightness.h"

uint8_t UsbDebugInfo[USB_GENERIC_HID_OUT_BUFFER_LENGTH];

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
        case UsbCommandId_WriteUserConfiguration:
            UsbCommand_WriteConfig(false);
            break;
        case UsbCommandId_ApplyConfig:
            UsbCommand_ApplyConfig();
            break;
        case UsbCommandId_SetLedPwmBrightness:
            UsbCommand_SetLedPwmBrightness();
            break;
        case UsbCommandId_GetAdcValue:
            UsbCommand_GetAdcValue();
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
