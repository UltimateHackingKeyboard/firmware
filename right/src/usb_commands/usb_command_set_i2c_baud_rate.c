#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_i2c_baud_rate.h"
#include "init_peripherals.h"

void UsbCommand_SetI2cBaudRate(void)
{
    uint32_t i2cBaudRate = GetUsbRxBufferUint32(1);
    I2cMainBusBaudRateBps = i2cBaudRate;
    ReinitI2cMainBus();
}
