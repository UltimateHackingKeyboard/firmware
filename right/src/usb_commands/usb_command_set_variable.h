#ifndef __USB_COMMAND_SET_VARIABLE_H__
#define __USB_COMMAND_SET_VARIABLE_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_SetVariable(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
