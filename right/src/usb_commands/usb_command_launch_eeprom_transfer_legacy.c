#include "fsl_common.h"
#include "usb_commands/usb_command_write_config.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"

void UsbCommand_LaunchEepromTransferLegacy(void)
{
    uint8_t legacyEepromTransferId = GET_USB_BUFFER_UINT8(1);
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
