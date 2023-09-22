#include "usb_protocol_handler.h"
#include "buffer.h"
#include "usb_commands/usb_command_get_device_property.h"
#include "usb_commands/usb_command_get_module_property.h"
#include "usb_commands/usb_command_reenumerate.h"
#include "usb_commands/usb_command_set_test_led.h"
#include "usb_commands/usb_command_write_config.h"
#include "usb_commands/usb_command_apply_config.h"
#include "usb_commands/usb_command_set_led_pwm_brightness.h"
#include "usb_commands/usb_command_get_adc_value.h"
#include "usb_commands/usb_command_launch_eeprom_transfer.h"
#include "usb_commands/usb_command_read_config.h"
#include "usb_commands/usb_command_get_device_state.h"
#include "usb_commands/usb_command_get_debug_buffer.h"
#include "usb_commands/usb_command_jump_to_module_bootloader.h"
#include "usb_commands/usb_command_send_kboot_command_to_module.h"
#include "usb_commands/usb_command_get_slave_i2c_errors.h"
#include "usb_commands/usb_command_set_i2c_baud_rate.h"
#include "usb_commands/usb_command_switch_keymap.h"
#include "usb_commands/usb_command_get_variable.h"
#include "usb_commands/usb_command_set_variable.h"
#include "usb_commands/usb_command_exec_macro_command.h"

void UsbProtocolHandler(void)
{
    bzero(GenericHidInBuffer, USB_GENERIC_HID_IN_BUFFER_LENGTH);
    uint8_t command = GetUsbRxBufferUint8(0);
    switch (command) {
        case UsbCommandId_GetDeviceProperty:
            UsbCommand_GetDeviceProperty();
            break;
        case UsbCommandId_Reenumerate:
            UsbCommand_Reenumerate();
            break;
        case UsbCommandId_JumpToModuleBootloader:
            UsbCommand_JumpToModuleBootloader();
            break;
        case UsbCommandId_SendKbootCommandToModule:
            UsbCommand_SendKbootCommandToModule();
            break;
        case UsbCommandId_ReadConfig:
            UsbCommand_ReadConfig();
            break;
        case UsbCommandId_WriteHardwareConfig:
            UsbCommand_WriteConfig(ConfigBufferId_HardwareConfig);
            break;
        case UsbCommandId_WriteStagingUserConfig:
            UsbCommand_WriteConfig(ConfigBufferId_StagingUserConfig);
            break;
        case UsbCommandId_ApplyConfig:
            UsbCommand_ApplyConfig();
            break;
        case UsbCommandId_LaunchEepromTransfer:
            UsbCommand_LaunchEepromTransfer();
            break;
        case UsbCommandId_GetDeviceState:
            UsbCommand_GetKeyboardState();
            break;
        case UsbCommandId_SetTestLed:
            UsbCommand_SetTestLed();
            break;
        case UsbCommandId_GetDebugBuffer:
            UsbCommand_GetDebugBuffer();
            break;
        case UsbCommandId_GetAdcValue:
            UsbCommand_GetAdcValue();
            break;
        case UsbCommandId_SetLedPwmBrightness:
            UsbCommand_SetLedPwmBrightness();
            break;
        case UsbCommandId_GetModuleProperty:
            UsbCommand_GetModuleProperty();
            break;
        case UsbCommandId_GetSlaveI2cErrors:
            UsbCommand_GetSlaveI2cErrors();
            break;
        case UsbCommandId_SetI2cBaudRate:
            UsbCommand_SetI2cBaudRate();
            break;
        case UsbCommandId_SwitchKeymap:
            UsbCommand_SwitchKeymap();
            break;
        case UsbCommandId_GetVariable:
            UsbCommand_GetVariable();
            break;
        case UsbCommandId_SetVariable:
            UsbCommand_SetVariable();
            break;
        case UsbCommandId_ExecMacroCommand:
            UsbCommand_ExecMacroCommand();
            break;
        default:
            SetUsbTxBufferUint8(0, UsbStatusCode_InvalidCommand);
            break;
    }
}

uint8_t GetUsbRxBufferUint8(uint32_t offset)
{
    return GetBufferUint8(GenericHidOutBuffer, offset);
}

uint16_t GetUsbRxBufferUint16(uint32_t offset)
{
    return GetBufferUint16(GenericHidOutBuffer, offset);
}

uint32_t GetUsbRxBufferUint32(uint32_t offset)
{
    return GetBufferUint32(GenericHidOutBuffer, offset);
}

void SetUsbTxBufferUint8(uint32_t offset, uint8_t value)
{
    SetBufferUint8(GenericHidInBuffer, offset, value);
}

void SetUsbTxBufferUint16(uint32_t offset, uint16_t value)
{
    SetBufferUint16(GenericHidInBuffer, offset, value);
}

void SetUsbTxBufferUint32(uint32_t offset, uint32_t value)
{
    SetBufferUint32(GenericHidInBuffer, offset, value);
}
