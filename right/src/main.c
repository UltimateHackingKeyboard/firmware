#include "config.h"
#include "init_clock.h"
#include "init_peripherals.h"
#include "usb_composite_device.h"
#include "slave_scheduler.h"
#include "bus_pal_hardware.h"
#include "command.h"
#include "bootloader/wormhole.h"
#include "eeprom.h"
#include "key_scanner.h"
#include "usb_commands/usb_command_apply_config.h"
#include "peripherals/reset_button.h"
#include "usb_report_updater.h"

bool IsEepromInitialized = false;
bool IsConfigInitialized = false;

void userConfigurationReadFinished(void)
{
    IsEepromInitialized = true;
}

void hardwareConfigurationReadFinished(void)
{
    EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_StagingUserConfig, userConfigurationReadFinished);
}

void main(void)
{
    InitClock();
    InitPeripherals();

    if (!RESET_BUTTON_IS_PRESSED) {
        EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_HardwareConfig, hardwareConfigurationReadFinished);
    }

    if (Wormhole.magicNumber == WORMHOLE_MAGIC_NUMBER && Wormhole.enumerationMode == EnumerationMode_BusPal) {
        Wormhole.magicNumber = 0;
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
