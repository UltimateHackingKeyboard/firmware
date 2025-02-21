#ifndef __ZEPHYR__
#include "fsl_common.h"
#endif

#include "usb_commands/usb_command_get_module_property.h"
#include "usb_protocol_handler.h"
#include "slot.h"
#include "slave_drivers/uhk_module_driver.h"
#include <string.h>
#include "utils.h"
#include "versioning.h"

void UsbCommand_GetModuleProperty(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    slot_t slotId = GetUsbRxBufferUint8(1);

    uint8_t moduleDriverId = UhkModuleSlaveDriver_SlotIdToDriverId(slotId);

    if (!IS_VALID_MODULE_SLOT(slotId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_GetModuleProperty_InvalidModuleSlotId);
    }

    if (!ModuleConnectionStates[moduleDriverId].moduleId || ModuleConnectionStates[moduleDriverId].moduleId == ModuleId_TouchpadRight) {
        // return empty buffer
        return;
    }

    module_property_id_t modulePropertyId = GetUsbRxBufferUint8(2);
    switch (modulePropertyId) {
        case ModulePropertyId_VersionNumbers: {
            uhk_module_state_t *moduleState = UhkModuleStates + moduleDriverId;
            GenericHidInBuffer[1] = moduleState->moduleId;
            memcpy(GenericHidInBuffer + 2, &moduleState->moduleProtocolVersion, sizeof(version_t));
            memcpy(GenericHidInBuffer + 8, &moduleState->firmwareVersion, sizeof(version_t));
            break;
        }
        case ModulePropertyId_GitTag: {
            uhk_module_state_t *moduleState = UhkModuleStates + moduleDriverId;
            Utils_SafeStrCopy(((char*)GenericHidInBuffer) + 1, moduleState->gitTag, USB_GENERIC_HID_IN_BUFFER_LENGTH - 1);
            break;
        }
        case ModulePropertyId_GitRepo: {
            uhk_module_state_t *moduleState = UhkModuleStates + moduleDriverId;
            Utils_SafeStrCopy(((char*)GenericHidInBuffer) + 1, moduleState->gitRepo, USB_GENERIC_HID_IN_BUFFER_LENGTH - 1);
            break;
        }
        case ModulePropertyId_RemoteFirmwareChecksumBySlotId: {
            uhk_module_state_t *moduleState = UhkModuleStates + moduleDriverId;
            Utils_SafeStrCopy(((char*)GenericHidInBuffer) + 1, moduleState->firmwareChecksum, MD5_CHECKSUM_LENGTH + 1);
            break;
        }
    }
}
