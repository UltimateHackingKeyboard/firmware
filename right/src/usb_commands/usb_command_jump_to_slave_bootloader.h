#ifndef __USB_COMMAND_JUMP_TO_SLAVE_BOOTLOADER_H__
#define __USB_COMMAND_JUMP_TO_SLAVE_BOOTLOADER_H__

// Functions:

    void UsbCommand_JumpToSlaveBootloader(void);

// Typedefs:

    typedef enum {
        UsbStatusCode_JumpToSlaveBootloader_InvalidModuleDriverId = 2,
    } usb_status_code_jump_to_slave_bootloader_t;

#endif
