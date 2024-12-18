#ifndef __USB_COMMAND_SEND_KBOOT_COMMAND_TO_MODULE_H__
#define __USB_COMMAND_SEND_KBOOT_COMMAND_TO_MODULE_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_SendKbootCommandToModule(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
