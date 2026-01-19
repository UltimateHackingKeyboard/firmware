#include <strings.h>
#include "config_parser/config_globals.h"
#include "macros/status_buffer.h"
#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_get_device_state.h"
#include "usb_commands/usb_command_read_config.h"
#include "usb_commands/usb_command_reenumerate.h"
#include "usb_commands/usb_command_write_config.h"
#include "usb_commands/usb_command_apply_config.h"
#include "usb_commands/usb_command_get_debug_buffer.h"
#include "usb_commands/usb_command_exec_macro_command.h"
#include "usb_commands/usb_command_get_device_property.h"
#include "usb_commands/usb_command_get_variable.h"
#include "usb_commands/usb_command_set_variable.h"
#include "usb_commands/usb_command_switch_keymap.h"
#include "usb_commands/usb_command_launch_storage_transfer.h"
#include "usb_commands/usb_command_get_module_property.h"
#include "usb_commands/usb_command_exec_shell_command.h"

#ifdef __ZEPHYR__
#include "usb_commands/usb_command_draw_oled.h"
#include "usb_commands/usb_command_read_oled.h"
#include "usb_commands/usb_command_pairing.h"
#include "usb_commands/usb_command_erase_ble_settings.h"
#include "bt_conn.h"
#else
#include "usb_commands/usb_command_set_test_led.h"
#include "usb_commands/usb_command_set_led_pwm_brightness.h"
#include "usb_commands/usb_command_set_uhk60_led_state.h"
#include "usb_commands/usb_command_get_adc_value.h"
#include "usb_commands/usb_command_jump_to_module_bootloader.h"
#include "usb_commands/usb_command_send_kboot_command_to_module.h"
#include "usb_commands/usb_command_get_slave_i2c_errors.h"
#include "usb_commands/usb_command_set_i2c_baud_rate.h"
#endif

void UsbProtocolHandler(uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    bzero(GenericHidInBuffer, USB_GENERIC_HID_IN_BUFFER_LENGTH);

    uint8_t command = GetUsbRxBufferUint8(0);
    switch (command) {
        case UsbCommandId_GetDeviceProperty:
            UsbCommand_GetDeviceProperty(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_Reenumerate:
            UsbCommand_Reenumerate(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_ReadConfig:
            UsbCommand_ReadConfig(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_WriteHardwareConfig:
            UsbCommand_WriteConfig(ConfigBufferId_HardwareConfig, GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_WriteStagingUserConfig:
            UsbCommand_WriteConfig(ConfigBufferId_StagingUserConfig, GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_ApplyConfig:

#ifdef __ZEPHYR__
            UsbCommand_ValidateAndApplyConfigAsync(GenericHidOutBuffer, GenericHidInBuffer);
#else
            UsbCommand_ValidateAndApplyConfigSync(GenericHidOutBuffer, GenericHidInBuffer);
#endif
            break;
        case UsbCommandId_GetDeviceState:
            UsbCommand_GetKeyboardState(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_GetDebugBuffer:
            UsbCommand_GetDebugBuffer(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_SwitchKeymap:
            UsbCommand_SwitchKeymap(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_GetVariable:
            UsbCommand_GetVariable(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_SetVariable:
            UsbCommand_SetVariable(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_ExecMacroCommand:
            UsbCommand_ExecMacroCommand(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_LaunchStorageTransfer:
            UsbCommand_LaunchStorageTransfer(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_GetModuleProperty:
            UsbCommand_GetModuleProperty(GenericHidOutBuffer, GenericHidInBuffer);
            break;
#ifdef __ZEPHYR__
        case UsbCommandId_ExecShellCommand:
            UsbCommand_ExecShellCommand(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_DrawOled:
            UsbCommand_DrawOled(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_ReadOled:
            UsbCommand_ReadOled(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_GetPairingData:
            UsbCommand_GetPairingData(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_SetPairingData:
            UsbCommand_SetPairingData(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_PairCentral:
            UsbCommand_PairCentral(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_PairPeripheral:
            UsbCommand_PairPeripheral(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_UnpairAll:
            UsbCommand_Unpair(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_IsPaired:
            UsbCommand_IsPaired(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_EnterPairingMode:
            UsbCommand_EnterPairingMode(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_EraseBleSettings:
            UsbCommand_EraseAllSettings();
            break;
#else
        case UsbCommandId_JumpToModuleBootloader:
            UsbCommand_JumpToModuleBootloader(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_SendKbootCommandToModule:
            UsbCommand_SendKbootCommandToModule(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_SetTestLed:
            UsbCommand_SetTestLed(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_GetAdcValue:
            UsbCommand_GetAdcValue(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_SetLedPwmBrightness:
            UsbCommand_SetLedPwmBrightness(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_GetSlaveI2cErrors:
            UsbCommand_GetSlaveI2cErrors(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_SetI2cBaudRate:
            UsbCommand_SetI2cBaudRate(GenericHidOutBuffer, GenericHidInBuffer);
            break;
        case UsbCommandId_SetUhk60LedState:
            UsbCommand_SetUhk60LedState(GenericHidOutBuffer, GenericHidInBuffer);
            break;
#endif
        default:
            SetUsbTxBufferUint8(0, UsbStatusCode_InvalidCommand);
            break;
    }
    if (GenericHidInBuffer[0] != UsbStatusCode_Success) {
        Macros_ReportErrorPrintf(NULL, "Usb protocol command %d failed with: %d\n", command, GenericHidInBuffer[0]);
    }

    bzero(GenericHidOutBuffer, USB_GENERIC_HID_OUT_BUFFER_LENGTH);
}

#ifdef __ZEPHYR__
bt_addr_le_t GetBufferBleAddress(const uint8_t *GenericHidOutBuffer, uint32_t offset) {
    bt_addr_le_t addr;
    for (uint8_t i = 0; i < BLE_ADDR_LEN; i++) {
        addr.a.val[i] = GenericHidOutBuffer[offset + i];
    }
    addr.type = addr.a.val[0] & 0x01;
    return addr;
}

void SetBufferBleAddress(uint8_t *GenericHidInBuffer, uint32_t offset, const bt_addr_le_t* addr) {
    for (uint8_t i = 0; i < BLE_ADDR_LEN; i++) {
        GenericHidInBuffer[offset + i] = addr->a.val[i];
    }
}


// void UsbCommand_EraseBleSettings(void) {
//     Settings_Erase();
// }

#endif
