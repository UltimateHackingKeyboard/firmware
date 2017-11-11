#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_send_kboot_command.h"
#include "slave_drivers/kboot_driver.h"

void UsbCommand_SendKbootCommand(void)
{
    KbootDriverState.phase = 0;
    KbootDriverState.i2cAddress  = GetUsbRxBufferUint8(2);
    KbootDriverState.commandType = GetUsbRxBufferUint8(1); // Command should be set last.
}
