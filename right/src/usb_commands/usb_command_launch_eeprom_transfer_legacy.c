#include "fsl_common.h"
#include "usb_commands/usb_command_launch_eeprom_transfer_legacy.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_LaunchEepromTransferLegacy(void)
{
    uint8_t legacyEepromTransferId = GetUsbRxBufferUint8(1);
    switch (legacyEepromTransferId) {
    case 0:
        EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_HardwareConfig, NULL);
        break;
    case 1:
        EEPROM_LaunchTransfer(EepromOperation_Write, ConfigBufferId_HardwareConfig, NULL);
        break;
    case 2:
        EEPROM_LaunchTransfer(EepromOperation_Read, ConfigBufferId_ValidatedUserConfig, NULL);
        break;
    case 3:
        EEPROM_LaunchTransfer(EepromOperation_Write, ConfigBufferId_ValidatedUserConfig, NULL);
        break;
    }
}
