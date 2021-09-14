#include "config.h"
#include "init_clock.h"
#include "init_peripherals.h"
#include "usb_composite_device.h"
#include "slave_scheduler.h"
#include "bus_pal_hardware.h"
#include "command.h"
#include "eeprom.h"
#include "right_key_matrix.h"
#include "usb_commands/usb_command_apply_config.h"
#include "peripherals/reset_button.h"
#include "config_parser/config_globals.h"
#include "usb_report_updater.h"
#include "ledmap.h"

static bool IsEepromInitialized = false;
static bool IsConfigInitialized = false;

static void userConfigurationReadFinished(void)
{
    IsEepromInitialized = true;
}

static void hardwareConfigurationReadFinished(void)
{
    InitLedLayout();
    if (IsFactoryResetModeEnabled) {
        HardwareConfig->signatureLength = HARDWARE_CONFIG_SIGNATURE_LENGTH;
        strncpy(HardwareConfig->signature, "FTY", HARDWARE_CONFIG_SIGNATURE_LENGTH);
    }
    EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_StagingUserConfig, userConfigurationReadFinished);
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

        while (1) {
            if (!IsConfigInitialized && IsEepromInitialized) {
                UsbCommand_ApplyConfig();
                IsConfigInitialized = true;
            }
            KeyMatrix_ScanRow(&RightKeyMatrix);
            ++MatrixScanCounter;
            UpdateUsbReports();
            __WFI();
        }
    }
}
