#include "config.h"
#include "timer.h"
#include "init_clock.h"
#include "init_peripherals.h"
#include "lufa/HIDClassCommon.h"
#include "segment_display.h"
#include "usb_commands/usb_command_exec_macro_command.h"
#include "usb_composite_device.h"
#include "slave_scheduler.h"
#include "bus_pal_hardware.h"
#include "command.h"
#include "eeprom.h"
#include "right_key_matrix.h"
#include "usb_commands/usb_command_apply_config.h"
#include "peripherals/reset_button.h"
#include "config_parser/config_globals.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_report_updater.h"
#include "macro_events.h"
#include "macros_shortcut_parser.h"
#include "macros_keyid_parser.h"
#include "ledmap.h"
#include "debug.h"
#include "event_scheduler.h"

static bool IsEepromInitialized = false;
static bool IsConfigInitialized = false;

static void userConfigurationReadFinished(void)
{
    IsEepromInitialized = true;
}

static void hardwareConfigurationReadFinished(void)
{
    Ledmap_InitLedLayout();
    if (IsFactoryResetModeEnabled) {
        HardwareConfig->signatureLength = HARDWARE_CONFIG_SIGNATURE_LENGTH;
        strncpy(HardwareConfig->signature, "FTY", HARDWARE_CONFIG_SIGNATURE_LENGTH);
    }
    EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_StagingUserConfig, userConfigurationReadFinished);
}

static void initConfig()
{
    while (!IsConfigInitialized) {
        if (IsEepromInitialized) {
            UsbCommand_ApplyConfig();
            ShortcutParser_initialize();
            KeyIdParser_initialize();
            Macros_Initialize();
            IsConfigInitialized = true;
        } else {
            __WFI();
        }
    }
}

static void sendFirstReport()
{
    bool success = false;
    while (!success) {
        UsbReportUpdateSemaphore |= 1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX;
        usb_status_t status = UsbBasicKeyboardAction();
        if (status != kStatus_USB_Success) {
            UsbReportUpdateSemaphore &= ~(1 << USB_BASIC_KEYBOARD_INTERFACE_INDEX);
            __WFI();
        } else {
            success = true;
        }
    }
    while (UsbReportUpdateSemaphore) {
            __WFI();
    }
}

int main(void)
{
    InitClock();
    InitPeripherals();

    IsFactoryResetModeEnabled = RESET_BUTTON_IS_PRESSED;

    EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_HardwareConfig, hardwareConfigurationReadFinished);

    if (IsBusPalOn) {
        init_hardware();
        handleUsbBusPalCommand();
    } else {
        InitSlaveScheduler();
        KeyMatrix_Init(&RightKeyMatrix);
        InitUsb();
        initConfig();
        sendFirstReport();

        while (1) {
            if (UsbBasicKeyboard_ProtocolChanged) {
                UsbBasicKeyboard_HandleProtocolChange();
            }
            if (UsbMacroCommandWaitingForExecution) {
                UsbMacroCommand_ExecuteSynchronously();
            }
            if (MacroEvent_ScrollLockStateChanged || MacroEvent_NumLockStateChanged || MacroEvent_CapsLockStateChanged) {
                MacroEvent_ProcessStateKeyEvents();
            }

            KeyMatrix_ScanRow(&RightKeyMatrix);
            ++MatrixScanCounter;
            UpdateUsbReports();

            if (EventScheduler_IsActive) {
                EventScheduler_Process();
            }
            if (SegmentDisplay_NeedsUpdate) {
                SegmentDisplay_Update();
            }
            __WFI();
        }
    }
}
