#include "usb_protocol_handler.h"
#include <string.h>

#define USB_SHELL_COMMAND_MAX_LEN (USB_COMMAND_BUFFER_LENGTH - 1)

#include "shell.h"

void UsbCommand_ExecShellCommand(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    // Null-terminate the command in-place to avoid extra buffer allocation
    ((char*)GenericHidOutBuffer)[USB_COMMAND_BUFFER_LENGTH - 1] = '\0';
    Shell_Execute((const char*)GenericHidOutBuffer + 1, "usb");
    SetUsbTxBufferUint8(0, UsbStatusCode_Success);
}
