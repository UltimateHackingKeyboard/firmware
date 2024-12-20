#ifndef __USB_COMMAND_EXEC_MACRO_COMMAND_H__
#define __USB_COMMAND_EXEC_MACRO_COMMAND_H__

// Includes:

    #include "config_parser/config_globals.h"
    #include "usb_interfaces/usb_interface_generic_hid.h"

// Typedefs:

// Variables:

    extern char UsbMacroCommand[USB_GENERIC_HID_OUT_BUFFER_LENGTH+1];
    extern uint8_t UsbMacroCommandLength;

// Functions:

    void UsbMacroCommand_ExecuteSynchronously();
    void UsbCommand_ExecMacroCommand(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
