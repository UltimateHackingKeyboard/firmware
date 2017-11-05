#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_jump_to_slave_bootloader.h"
#include "slave_drivers/uhk_module_driver.h"

void UsbCommand_JumpToSlaveBootloader(void)
{
    uint8_t uhkModuleDriverId = GET_USB_BUFFER_UINT8(1);

    if (uhkModuleDriverId >= UHK_MODULE_MAX_COUNT) {
        SET_USB_BUFFER_UINT8(0, UsbStatusCode_JumpToSlaveBootloader_InvalidModuleDriverId);
        return;
    }

    UhkModuleStates[uhkModuleDriverId].jumpToBootloader = true;
}
