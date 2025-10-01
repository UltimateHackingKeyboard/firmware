#ifndef __USB_COMMAND_READ_OLED_H__
#define __USB_COMMAND_READ_OLED_H__

// Includes:

    #include "config_parser/config_globals.h"

// Typedefs:

// Variables:

// Functions:

    void UsbCommand_ReadOled(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif