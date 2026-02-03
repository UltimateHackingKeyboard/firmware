#ifndef __ZEPHYR__
#include "fsl_common.h"
#endif
#include "macros/core.h"
#include "macros/status_buffer.h"
#include "usb_commands/usb_command_exec_macro_command.h"
#include "usb_interfaces/usb_interface_generic_hid.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "utils.h"
#include <string.h>
#include "debug.h"

char UsbMacroCommand[USB_GENERIC_HID_OUT_BUFFER_LENGTH+1];
uint8_t UsbMacroCommandLength = 0;
key_state_t dummyState;
key_press_info_t dummyPress = {.keyState = &dummyState};

static void requestExecution(const uint8_t *GenericHidOutBuffer)
{
    uint8_t len = Utils_SafeStrCopy(UsbMacroCommand, ((char*)GenericHidOutBuffer) + 1, USB_GENERIC_HID_OUT_BUFFER_LENGTH - 1);
    UsbMacroCommandLength = len;

    EventVector_Set(EventVector_UsbMacroCommandWaitingForExecution);

#ifdef __ZEPHYR__
    Main_Wake();
#endif
}

static bool canExecute()
{
    if (EventVector_IsSet(EventVector_UsbMacroCommandWaitingForExecution)) {
        return false;
    }

    if (Macros_MacroHasActiveInstance(MacroIndex_InlineMacro)) {
        return false;
    }

    return true;
}

void UsbMacroCommand_ExecuteSynchronously()
{
    Macros_StartMacro(MacroIndex_InlineMacro, &dummyPress, 0, 255, MacroIndex_None, false, UsbMacroCommand);
    EventVector_Unset(EventVector_UsbMacroCommandWaitingForExecution);
}

void UsbCommand_ExecMacroCommand(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    if (canExecute()) {
        requestExecution(GenericHidOutBuffer);
    } else {
        SetUsbTxBufferUint8(0, UsbStatusCode_Busy);
        Macros_ReportErrorPrintf(NULL, "Another usb macro command executing, cannot execute now.\n");
    }
}
