#include "usb_protocol_handler.h"
#include <string.h>

#define USB_SHELL_COMMAND_MAX_LEN (USB_GENERIC_HID_OUT_BUFFER_LENGTH - 1)

#ifdef __ZEPHYR__
#include "shell.h"
#endif

void UsbCommand_ExecShellCommand(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
#ifdef __ZEPHYR__
    // Null-terminate the command in-place to avoid extra buffer allocation
    ((char*)GenericHidOutBuffer)[USB_GENERIC_HID_OUT_BUFFER_LENGTH - 1] = '\0';
    Shell_Execute((const char*)GenericHidOutBuffer + 1, "usb");
    SetUsbTxBufferUint8(0, UsbStatusCode_Success);
#else
    (void)GenericHidOutBuffer;
    (void)GenericHidInBuffer;
#endif
}
