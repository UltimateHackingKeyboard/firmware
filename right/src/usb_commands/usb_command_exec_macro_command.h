#ifndef __USB_COMMAND_EXEC_MACRO_COMMAND_H__
#define __USB_COMMAND_EXEC_MACRO_COMMAND_H__

// Includes:

    #include "config_parser/config_globals.h"

// Typedefs:

    #define USB_COMMAND_MACRO_COMMAND_MAX_LENGTH 63


// Variables:

extern char UsbMacroCommand[USB_COMMAND_MACRO_COMMAND_MAX_LENGTH+1];
extern uint8_t UsbMacroCommandLength;
extern bool UsbMacroCommandWaitingForExecution;

// Functions:

    void UsbMacroCommand_ExecuteSynchronously();
    void UsbCommand_ExecMacroCommand();

#endif
