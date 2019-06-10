#include "fsl_common.h"
#include "usb_commands/usb_command_get_module_property.h"
#include "usb_protocol_handler.h"
#include "slot.h"
#include "slave_drivers/uhk_module_driver.h"

void UsbCommand_GetModuleProperty()
{
    slot_t slotId = GetUsbRxBufferUint8(1);

    if (!IS_VALID_MODULE_SLOT(slotId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_GetModuleProperty_InvalidModuleSlotId);
    }

    module_property_id_t modulePropertyId = GetUsbRxBufferUint8(2);
    switch (modulePropertyId) {
        case ModulePropertyId_VersionNumbers: {
            uint8_t moduleDriverId = UhkModuleSlaveDriver_SlotIdToDriverId(slotId);
            uhk_module_state_t *moduleState = UhkModuleStates + moduleDriverId;
            GenericHidOutBuffer[1] = moduleState->moduleId;
            memcpy(GenericHidOutBuffer + 2, &moduleState->moduleProtocolVersion, sizeof(version_t));
            memcpy(GenericHidOutBuffer + 8, &moduleState->firmwareVersion, sizeof(version_t));
            break;
        }
    }
}
