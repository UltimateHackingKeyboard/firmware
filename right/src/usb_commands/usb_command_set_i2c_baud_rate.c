#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_i2c_baud_rate.h"
#include "init_peripherals.h"
#include "fsl_i2c.h"

void UsbCommand_SetI2cBaudRate(void)
{
    uint32_t i2cBaudRate = GetUsbRxBufferUint32(1);
    ChangeI2cBaudRate(i2cBaudRate);
}
