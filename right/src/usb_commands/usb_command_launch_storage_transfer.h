#ifndef __USB_COMMAND_LAUNCH_STORAGE_TRANSFER_H__
#define __USB_COMMAND_LAUNCH_STORAGE_TRANSFER_H__

// Typedef

    typedef enum {
        UsbStatusCode_LaunchStorageTransferInvalidEepromOperation = 2,
        UsbStatusCode_LaunchStorageTransferInvalidConfigBufferId = 3,
        UsbStatusCode_LaunchStorageTransferTransferError = 4,
    } usb_status_code_launch_storage_transfer_t;

// Functions:

    void UsbCommand_LaunchStorageTransfer(void);

#endif
