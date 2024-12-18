#ifndef __USB_COMMAND_JUMP_TO_MODULE_BOOTLOADER_H__
#define __USB_COMMAND_JUMP_TO_MODULE_BOOTLOADER_H__

// Includes:

    #include <stdint.h>

// Functions:

    void UsbCommand_JumpToModuleBootloader(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer);

// Typedefs:

    typedef enum {
        UsbStatusCode_JumpToModuleBootloader_InvalidSlaveSlotId = 2,
    } usb_status_code_jump_to_module_bootloader_t;

#endif
