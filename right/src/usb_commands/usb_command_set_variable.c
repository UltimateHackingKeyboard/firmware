#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_variable.h"
#include "key_matrix.h"
#include "test_switches.h"
#include "usb_report_updater.h"
#include "config_manager.h"
#include "ledmap.h"

void UsbCommand_SetVariable(void)
{
    usb_variable_id_t variableId = GetUsbRxBufferUint8(1);

    switch (variableId) {
        case UsbVariable_TestSwitches:
            if (GetUsbRxBufferUint8(2)) {
                TestSwitches = true;
                TestSwitches_Activate();
                Ledmap_ActivateTestLedMode(true);
            } else {
                Ledmap_ActivateTestLedMode(false);
            }
            break;
        case UsbVariable_TestUsbStack:
            TestUsbStack = GetUsbRxBufferUint8(2);
            break;
        case UsbVariable_DebounceTimePress:
            Cfg.DebounceTimePress = GetUsbRxBufferUint8(2);
            break;
        case UsbVariable_DebounceTimeRelease:
            Cfg.DebounceTimeRelease = GetUsbRxBufferUint8(2);
            break;
        case UsbVariable_UsbReportSemaphore:
            UsbReportUpdateSemaphore = GetUsbRxBufferUint8(2);
            break;
        case UsbVariable_StatusBuffer:
            break;
    }
}
