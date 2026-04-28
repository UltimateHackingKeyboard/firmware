#include <string.h>
#include "usb_commands/usb_command_write_module_firmware.h"
#include "usb_commands/usb_command_write_config.h"
#include "usb_protocol_handler.h"
#include "config_parser/config_globals.h"

void UsbCommand_WriteModuleFirmware(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    UsbCommand_WriteConfig(ConfigBufferId_ModuleFirmware, GenericHidOutBuffer, GenericHidInBuffer);
}
