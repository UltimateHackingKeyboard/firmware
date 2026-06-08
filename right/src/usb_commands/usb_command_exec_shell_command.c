#include "usb_protocol_handler.h"
#include <string.h>
#include "logger.h"
#include "timer.h"

#define USB_SHELL_COMMAND_MAX_LEN (USB_COMMAND_BUFFER_LENGTH - 1)

#ifdef __ZEPHYR__
#include "shell/shell_uhk.h"
#endif

void UsbCommand_ExecShellCommand(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
#ifdef __ZEPHYR__
    // Null-terminate the command in-place to avoid extra buffer allocation
    ((char*)GenericHidOutBuffer)[USB_COMMAND_BUFFER_LENGTH - 1] = '\0';
    Shell_Input((const char*)GenericHidOutBuffer + 1);
    // Shell_Execute((const char*)GenericHidOutBuffer + 1, NULL /* don't log this */);
    SetUsbTxBufferUint8(0, UsbStatusCode_Success);
#else
    static uint32_t lastTime = 0;
    uint32_t currentTime = Timer_GetCurrentTime();
    if (currentTime - lastTime > 1000) {
        currentTime = 0;
        LogU("uhk60$: only output is supported for uhk60.");
    }
#endif
}
