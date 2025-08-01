#ifndef __USB_COMMAND_GET_DEBUG_BUFFER_H__
#define __USB_COMMAND_GET_DEBUG_BUFFER_H__

// Includes:

    #include "usb_interfaces/usb_interface_generic_hid.h"

// Variables:

// Functions:

    void UsbCommand_GetDebugBuffer(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
