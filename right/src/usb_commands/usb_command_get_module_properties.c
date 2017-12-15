#include "fsl_common.h"
#include "usb_commands/usb_command_get_module_properties.h"
#include "usb_protocol_handler.h"
#include "slot.h"
#include "slave_drivers/uhk_module_driver.h"

void UsbCommand_GetModuleProperties()
{
    slot_t slotId = GetUsbRxBufferUint8(1);

    if (!IS_VALID_MODULE_SLOT(slotId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_ReadConfig_InvalidModuleSlotId);
    }

    uint8_t moduleDriverId = SLOT_ID_TO_UHK_MODULE_DRIVER_ID(slotId);
    uhk_module_state_t *moduleState = UhkModuleStates + moduleDriverId;

    GenericHidOutBuffer[1] = moduleState->moduleId;
    memcpy(GenericHidOutBuffer + 2, &moduleState->moduleProtocolVersion, sizeof(version_t));
    memcpy(GenericHidOutBuffer + 2 + sizeof(version_t), &moduleState->firmwareVersion, sizeof(version_t));
}
