#include "usb_protocol_handler.h"
#include "slave_drivers/kboot_driver.h"

void UsbCommand_SendKbootCommand(void)
{
    KbootDriverState.phase = 0;
    KbootDriverState.i2cAddress = GenericHidInBuffer[2];
    KbootDriverState.commandType = GenericHidInBuffer[1]; // Command should be set last.
}
