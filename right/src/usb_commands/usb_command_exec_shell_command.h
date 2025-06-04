#ifndef __USB_COMMAND_EXEC_SHELL_COMMAND_H__
#define __USB_COMMAND_EXEC_SHELL_COMMAND_H__

#include <stdint.h>

void UsbCommand_ExecShellCommand(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif 