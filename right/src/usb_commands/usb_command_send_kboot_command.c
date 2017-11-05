#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_send_kboot_command.h"
#include "slave_drivers/kboot_driver.h"

void UsbCommand_SendKbootCommand(void)
{
    KbootDriverState.phase = 0;
    KbootDriverState.i2cAddress  = GET_USB_BUFFER_UINT8(2);
    KbootDriverState.commandType = GET_USB_BUFFER_UINT8(1); // Command should be set last.
}
