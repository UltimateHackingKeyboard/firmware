#include "usb_commands/usb_command_get_module_flash_state.h"
#include "usb_protocol_handler.h"
#include "module_flash.h"

void UsbCommand_GetModuleFlashState(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    SetUsbTxBufferUint8(1, ModuleFlashState);
    SetUsbTxBufferUint8(2, ModuleFlashErrorCode);
}
