#ifndef __USB_COMMAND_SET_I2C_BAUD_RATE__
#define __USB_COMMAND_SET_I2C_BAUD_RATE__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_SetI2cBaudRate(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
