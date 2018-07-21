#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_variable.h"
#include "key_matrix.h"
#include "test_mode.h"

void UsbCommand_SetVariable(void)
{
    usb_variable_id_t variableId = GetUsbRxBufferUint8(1);

    switch (variableId) {
        case UsbVariable_TestModeActive:
            if (GetUsbRxBufferUint8(2)) {
                TestModeActive = true;
                TestMode_Activate();
            }
            break;
        case UsbVariable_TestUsbStack:
            break;
        case UsbVariable_DebounceTimePress:
            DebounceTimePress = GetUsbRxBufferUint8(2);
            break;
        case UsbVariable_DebounceTimeRelease:
            DebounceTimeRelease = GetUsbRxBufferUint8(2);
            break;
    }
}
