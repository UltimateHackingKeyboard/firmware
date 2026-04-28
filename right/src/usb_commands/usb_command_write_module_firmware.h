#ifndef __USB_COMMAND_WRITE_MODULE_FIRMWARE_H__
#define __USB_COMMAND_WRITE_MODULE_FIRMWARE_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_WriteModuleFirmware(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
