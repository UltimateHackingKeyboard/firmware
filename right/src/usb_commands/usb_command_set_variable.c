#include <string.h>
#include "keymap.h"
#include "led_manager.h"
#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_set_variable.h"
#include "key_matrix.h"
#include "test_switches.h"
#include "usb_report_updater.h"
#include "config_manager.h"
#include "ledmap.h"
#include "layer.h"
#include "slot.h"
#include "event_scheduler.h"
#include "usb_interfaces/usb_interface_generic_hid.h"

#if defined(__ZEPHYR__) && DEVICE_IS_KEYBOARD
#include "keyboard/leds.h"
#endif

#ifdef __ZEPHYR__
#include "proxy_log_backend.h"
#include "state_sync.h"
#include "keyboard/oled/oled.h"
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
        case UsbVariable_FirmwareVersionCheckEnabled:
            #ifdef __ZEPHYR__
                StateSync_VersionCheckEnabled = GetUsbRxBufferUint8(2);

                if (StateSync_VersionCheckEnabled) {
                    EventScheduler_Reschedule(Timer_GetCurrentTime() + 1000, EventSchedulerEvent_CheckFwChecksums, "Reset left right link");
                }

            #endif
            break;
        case UsbVariable_LedOverride: {
            // Byte 2: UHK60 LED override flags
            Uhk60LedOverride = *(led_override_uhk60_t*)&GenericHidOutBuffer[2];
            // Byte 3: OLED override mode
#if DEVICE_HAS_OLED
            OledOverrideMode = GenericHidOutBuffer[3];
#endif

            printk("============= Writing oled %d\n", OledOverrideMode);
            // Bytes 4-35: per-key RGB override bitmap (32 bytes = 256 bits)
            // Deserialize into colorOverridden for ALL layers
            for (uint8_t slotIdx = 0; slotIdx < SLOT_COUNT; slotIdx++) {
                for (uint8_t inSlotIdx = 0; inSlotIdx < MAX_KEY_COUNT_PER_MODULE; inSlotIdx++) {
                    uint8_t keyId = slotIdx * 64 + inSlotIdx;
                    bool isOverridden = (GenericHidOutBuffer[4 + keyId / 8] >> (keyId % 8)) & 1;
                    for (uint8_t layerId = 0; layerId < LayerId_Count; layerId++) {
                        CurrentKeymap[layerId][slotIdx][inSlotIdx].colorOverridden = isOverridden;
                    }
                }
            }
#ifdef __ZEPHYR__
            // Sync all layers to the left half
            for (uint8_t layerId = 0; layerId < LayerId_Count; layerId++) {
                // TODO: optimize this somehow
                StateSync_UpdateLayer(layerId, true);
            }
#endif
            EventVector_Set(EventVector_LedManagerFullUpdateNeeded);
            break;
        }
        default:
            break;
    }
}
