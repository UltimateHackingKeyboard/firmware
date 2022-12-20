#include "fsl_common.h"
#include "macros.h"
#include "usb_commands/usb_command_exec_macro_command.h"
#include "usb_interfaces/usb_interface_generic_hid.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "utils.h"
#include <string.h>
#include "debug.h"

char UsbMacroCommand[USB_COMMAND_MACRO_COMMAND_MAX_LENGTH+1];
uint8_t UsbMacroCommandLength = 0;
bool UsbMacroCommandWaitingForExecution = false;
key_state_t dummyState;

static void requestExecution()
{
    Utils_SafeStrCopy(UsbMacroCommand, ((char*)GenericHidOutBuffer) + 1, sizeof(GenericHidOutBuffer)-1);
    UsbMacroCommandLength = strlen(UsbMacroCommand);

    UsbMacroCommandWaitingForExecution = true;
}

static bool canExecute()
{
    if (UsbMacroCommandWaitingForExecution) {
        return false;
    }

    // make sure there is no other instance running
    for(uint8_t j = 0; j < MACRO_STATE_POOL_SIZE; j++) {
        if(MacroState[j].ms.macroPlaying && MacroState[j].ms.currentMacroIndex == MacroIndex_UsbCmdReserved) {
            return false;
        }
    }

    return true;
}

void UsbMacroCommand_ExecuteSynchronously()
{
    Macros_StartMacro(MacroIndex_UsbCmdReserved, &dummyState, MacroIndex_None, false);
    UsbMacroCommandWaitingForExecution = false;
}

void UsbCommand_ExecMacroCommand()
{
    if (canExecute()) {
        requestExecution();
    }
}
