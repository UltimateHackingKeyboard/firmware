#include "usb_commands/usb_command_flash_module.h"
#include "usb_protocol_handler.h"
#include "module_flash.h"
#include "slot.h"

typedef enum {
    UsbStatusCode_FlashModule_InvalidSlotId = 2,
    UsbStatusCode_FlashModule_Busy          = 3,
} usb_status_code_flash_module_t;

void UsbCommand_FlashModule(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint8_t slotId = GetUsbRxBufferUint8(1);

    if (!IS_VALID_MODULE_SLOT(slotId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_FlashModule_InvalidSlotId);
        return;
    }

    if (ModuleFlashBusy) {
        SetUsbTxBufferUint8(0, UsbStatusCode_FlashModule_Busy);
        return;
    }

    ModuleFlashBusy = true;
    ModuleFlashErrorCode = 0;

    // TODO: Trigger actual K-boot flash sequence here.
    // For now, stub: immediately mark as done.
    ModuleFlashState = ModuleFlashState_Done;
    ModuleFlashBusy = false;
}
