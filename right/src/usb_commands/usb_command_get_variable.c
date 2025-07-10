#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_get_variable.h"
#include "key_matrix.h"
#include "test_switches.h"
#include "usb_report_updater.h"
#include "macros/core.h"
#include "config_manager.h"
#include "usb_interfaces/usb_interface_generic_hid.h"

#ifdef __ZEPHYR__
#include "proxy_log_backend.h"
#endif

void UsbCommand_GetVariable(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    usb_variable_id_t variableId = GetUsbRxBufferUint8(1);

    switch (variableId) {
        case UsbVariable_TestSwitches:
            SetUsbTxBufferUint8(1, TestSwitches);
            break;
        case UsbVariable_TestUsbStack:
            SetUsbTxBufferUint8(1, TestUsbStack);
            break;
        case UsbVariable_DebounceTimePress:
            SetUsbTxBufferUint8(1, Cfg.DebounceTimePress);
            break;
        case UsbVariable_DebounceTimeRelease:
            SetUsbTxBufferUint8(1, Cfg.DebounceTimeRelease);
            break;
        case UsbVariable_UsbReportSemaphore:
            SetUsbTxBufferUint8(1, UsbReportUpdateSemaphore);
            break;
        case UsbVariable_StatusBuffer:
            for (uint8_t i = 1; i < USB_GENERIC_HID_IN_BUFFER_LENGTH; i++) {
                char c = Macros_ConsumeStatusChar();
                SetUsbTxBufferUint8(i, c);
                if (c == '\0') {
                    break;
                }
            }
            break;
        case UsbVariable_ShellEnabled:
            #ifdef __ZEPHYR__
                SetUsbTxBufferUint8(1, ProxyLog_IsAttached);
            #endif
            break;
        case UsbVariable_ShellBuffer:
            #ifdef __ZEPHYR__
                ProxyLog_ConsumeLog(GenericHidInBuffer + 1, USB_GENERIC_HID_IN_BUFFER_LENGTH - 1);
            #endif
            break;
        case UsbVariable_LedAudioRegisters:
#if defined(__ZEPHYR__) && DEVICE_IS_KEYBOARD
            SetUsbTxBufferUint8(0, 1);
#endif
            break;
    }
}
