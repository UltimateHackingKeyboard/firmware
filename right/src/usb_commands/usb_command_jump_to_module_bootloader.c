#include "usb_protocol_handler.h"
#include "usb_commands/usb_command_jump_to_module_bootloader.h"
#include "slot.h"
#include "slave_drivers/uhk_module_driver.h"

void UsbCommand_JumpToModuleBootloader(void)
{
    uint8_t slotId = GetUsbRxBufferUint8(1);

    if (!IS_VALID_SLAVE_SLOT(slotId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_JumpToModuleBootloader_InvalidSlaveSlotId);
        return;
    }

    uint8_t uhkModuleDriverId = SLOT_ID_TO_UHK_MODULE_DRIVER_ID(slotId);
    UhkModuleStates[uhkModuleDriverId].phase = UhkModulePhase_JumpToBootloader;
}
