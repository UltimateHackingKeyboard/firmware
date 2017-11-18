#ifndef __USB_COMMAND_JUMP_TO_MODULE_BOOTLOADER_H__
#define __USB_COMMAND_JUMP_TO_MODULE_BOOTLOADER_H__

// Functions:

    void UsbCommand_JumpToModuleBootloader(void);

// Typedefs:

    typedef enum {
        UsbStatusCode_JumpToModuleBootloader_InvalidSlaveSlotId = 2,
    } usb_status_code_jump_to_module_bootloader_t;

#endif
