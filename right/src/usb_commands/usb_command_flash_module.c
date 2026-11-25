#include "usb_commands/usb_command_flash_module.h"
#include "usb_protocol_handler.h"
#include "module_flash.h"
#include "slot.h"
#include "slave_drivers/kboot_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "logger.h"

typedef enum {
    UsbStatusCode_FlashModule_InvalidSlotId = 2,
    UsbStatusCode_FlashModule_Busy          = 3,
} usb_status_code_flash_module_t;

void UsbCommand_FlashModule(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint8_t slotId = GetUsbRxBufferUint8(1);
    uint8_t bootloaderI2cAddress = GetUsbRxBufferUint8(2);

    if (!IS_VALID_MODULE_SLOT(slotId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_FlashModule_InvalidSlotId);
        return;
    }

    if (ModuleFlashBusy) {
        SetUsbTxBufferUint8(0, UsbStatusCode_FlashModule_Busy);
        return;
    }

    uint8_t driverId = UhkModuleSlaveDriver_SlotIdToDriverId(slotId);
    LogU("Kboot: FlashModule requested: slot %u, connected module id %u, bootloader address 0x%02x, validated firmware %u bytes\n",
         slotId, ModuleConnectionStates[driverId].moduleId, bootloaderI2cAddress, ModuleFirmwareValidatedSize);

    ModuleFlashBusy = true;
    ModuleFlashErrorCode = ModuleFlashError_None;
    ModuleFlashState = ModuleFlashState_TryingToReach;
    KbootDriverState.slotId = slotId;
    KbootDriverState.moduleBootloaderAddress = bootloaderI2cAddress;
    KbootDriverState.firmwareOffset = 0;
    KbootDriverState.firmwareSize = 0;
    KbootDriverState.phase = 0;
    KbootDriverState.command = KbootCommand_Flash;
}
