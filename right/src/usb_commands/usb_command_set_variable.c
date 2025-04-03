#include "keymap.h"
#include "led_manager.h"
#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_variable.h"
#include "key_matrix.h"
#include "test_switches.h"
#include "usb_report_updater.h"
#include "config_manager.h"
#include "ledmap.h"

#if defined(__ZEPHYR__) && DEVICE_IS_KEYBOARD
#include "keyboard/leds.h"
#endif

#ifdef __ZEPHYR__
#include "proxy_log_backend.h"
#endif

void UsbCommand_SetVariable(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    usb_variable_id_t variableId = GetUsbRxBufferUint8(1);

    switch (variableId) {
        case UsbVariable_TestSwitches:
            if (GetUsbRxBufferUint8(2)) {
                TestSwitches_Activate();
            } else {
                TestSwitches_Deactivate();
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
        case UsbVariable_LedAudioRegisters:
#if defined(__ZEPHYR__) && DEVICE_IS_KEYBOARD
            uint8_t phaseDelay = GetUsbRxBufferUint8(2);
            uint8_t spreadSpectrum = GetUsbRxBufferUint8(3);
            uint8_t pwmFrequency = GetUsbRxBufferUint8(4);
            UpdateLedAudioRegisters(phaseDelay, spreadSpectrum, pwmFrequency);
#endif
            break;
        case UsbVariable_ShellEnabled:
            #ifdef __ZEPHYR__
                ProxyLog_SetAttached(GetUsbRxBufferUint8(2));
            #endif
            break;
        default:
            break;
    }
}
