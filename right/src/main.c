#include "config.h"
#include "main.h"
#include "init_clock.h"
#include "init_peripherals.h"
#include "usb_composite_device.h"
#include "peripherals/led_driver.h"
#include "key_action.h"
#include "slave_scheduler.h"
#include "peripherals/test_led.h"
#include "usb_interfaces/usb_interface_basic_keyboard.h"
#include "usb_interfaces/usb_interface_media_keyboard.h"
#include "usb_protocol_handler.h"
#include "bus_pal_hardware.h"
#include "command.h"
#include "bootloader/wormhole.h"
#include "eeprom.h"
#include "right_key_matrix.h"
#include "key_scanner.h"
#include "key_states.h"

void updateUsbReports(void)
{
    if (!IsUsbBasicKeyboardReportSent) {
        return;
    }

#ifndef INTERRUPT_KEY_SCANNER
    KeyMatrix_Scan(&RightKeyMatrix);
#endif

    ResetActiveUsbBasicKeyboardReport();
    ResetActiveUsbMediaKeyboardReport();
    ResetActiveUsbSystemKeyboardReport();
    ResetActiveUsbMouseReport();

    UpdateActiveUsbReports();

    SwitchActiveUsbBasicKeyboardReport();
    SwitchActiveUsbMediaKeyboardReport();
    SwitchActiveUsbSystemKeyboardReport();
    SwitchActiveUsbMouseReport();

    IsUsbBasicKeyboardReportSent = false;
}

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
    EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_HardwareConfig, hardwareConfigurationReadFinished);

    if (Wormhole.magicNumber == WORMHOLE_MAGIC_NUMBER && Wormhole.enumerationMode == EnumerationMode_BusPal) {
        Wormhole.magicNumber = 0;
        init_hardware();
        handleUsbBusPalCommand();
    } else {
        InitSlaveScheduler();
        KeyMatrix_Init(&RightKeyMatrix);
#ifdef INTERRUPT_KEY_SCANNER
        InitKeyScanner();
#endif
        updateUsbReports();
        InitUsb();

        while (1) {
            if (!IsConfigInitialized && IsEepromInitialized) {
                ApplyConfig();
                IsConfigInitialized = true;
            }
            updateUsbReports();
            __WFI();
        }
    }
}
