#ifndef __USB_COMMAND_LAUNCH_STORAGE_TRANSFER_H__
#define __USB_COMMAND_LAUNCH_STORAGE_TRANSFER_H__

// Includes:

    #include <stdint.h>

// Typedef

    typedef enum {
        UsbStatusCode_LaunchStorageTransferInvalidStorageOperation = 2,
        UsbStatusCode_LaunchStorageTransferInvalidConfigBufferId = 3,
        UsbStatusCode_LaunchStorageTransferTransferError = 4,
    } usb_status_code_launch_storage_transfer_t;

// Functions:

    void UsbCommand_LaunchStorageTransfer(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

#endif
