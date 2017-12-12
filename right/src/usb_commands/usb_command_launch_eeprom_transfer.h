#ifndef __USB_COMMAND_LAUNCH_EEPROM_TRANSFER_H__
#define __USB_COMMAND_LAUNCH_EEPROM_TRANSFER_H__

// Typedef

    typedef enum {
        UsbStatusCode_LaunchEepromTransfer_InvalidEepromOperation = 2,
        UsbStatusCode_LaunchEepromTransfer_InvalidConfigBufferId = 3,
        UsbStatusCode_LaunchEepromTransfer_TransferError = 4,
    } usb_status_code_launch_eeprom_transfer_t;

// Functions:

    void UsbCommand_LaunchEepromTransfer(void);

#endif
