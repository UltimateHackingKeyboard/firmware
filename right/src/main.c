#include "config.h"
#include "init_clock.h"
#include "init_peripherals.h"
#include "usb_composite_device.h"
#include "slave_scheduler.h"
#include "bus_pal_hardware.h"
#include "command.h"
#include "eeprom.h"
#include "key_scanner.h"
#include "usb_commands/usb_command_apply_config.h"
#include "peripherals/reset_button.h"
#include "config_parser/config_globals.h"
#include "usb_report_updater.h"

static bool IsEepromInitialized = false;
static bool IsConfigInitialized = false;

static void userConfigurationReadFinished(void)
{
    IsEepromInitialized = true;
}

static void hardwareConfigurationReadFinished(void)
{
    EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_StagingUserConfig, userConfigurationReadFinished);
}

int main(void)
{
    InitClock();
    InitPeripherals();

    IsFactoryResetModeEnabled = RESET_BUTTON_IS_PRESSED;
    if (IsFactoryResetModeEnabled) {
        HardwareConfig->signatureLength = HARDWARE_CONFIG_SIGNATURE_LENGTH;
        strncpy(HardwareConfig->signature, "FTY", HARDWARE_CONFIG_SIGNATURE_LENGTH);
    } else {
        EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_HardwareConfig, hardwareConfigurationReadFinished);
    }

    if (IsBusPalOn) {
        init_hardware();
        handleUsbBusPalCommand();
    } else {
        InitSlaveScheduler();
        KeyMatrix_Init(&RightKeyMatrix);
        InitKeyScanner();
        UpdateUsbReports();
        InitUsb();

        while (1) {
            if (!IsConfigInitialized && IsEepromInitialized) {
                UsbCommand_ApplyConfig();
                IsConfigInitialized = true;
            }
            UpdateUsbReports();
            __WFI();
        }
    }
}
