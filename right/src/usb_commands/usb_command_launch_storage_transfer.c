#include "usb_commands/usb_command_launch_storage_transfer.h"
#include "event_scheduler.h"
#include "ledmap.h"
#include "usb_protocol_handler.h"
#include "config_parser/config_globals.h"
#include "macros/status_buffer.h"

#ifdef __ZEPHYR__
#include "flash.h"
typedef int32_t status_t;
#define MAKE_STATUS(group, code) ((((group)*100) + (code)))
enum _status_groups
{
    kStatusGroup_Generic = 0,
};
enum _generic_status
{
    kStatus_Success = MAKE_STATUS(kStatusGroup_Generic, 0),
    kStatus_Fail = MAKE_STATUS(kStatusGroup_Generic, 1),
};
#else
#include "fsl_common.h"
#include "eeprom.h"
#endif

void UsbCommand_LaunchStorageTransfer(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    storage_operation_t storageOperation = GetUsbRxBufferUint8(1);
    config_buffer_id_t configBufferId = GetUsbRxBufferUint8(2);

    if (!IsStorageOperationValid(storageOperation)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_LaunchStorageTransferInvalidStorageOperation);
    }

    if (!IsConfigBufferIdValid(configBufferId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_LaunchStorageTransferInvalidConfigBufferId);
    }

    if (storageOperation == StorageOperation_Write && configBufferId != ConfigBufferId_HardwareConfig) {
        config_buffer_t* buffer = ConfigBufferIdToConfigBuffer(configBufferId);
        if (!buffer->isValid) {
            SetUsbTxBufferUint8(0, UsbStatusCode_LaunchStorageTransferInvalidConfigBuffer);
            return;
        }
    }

#ifdef __ZEPHYR__
    status_t status = Flash_LaunchTransfer(storageOperation, configBufferId, NULL);
#else
    status_t status = EEPROM_LaunchTransfer(storageOperation, configBufferId, NULL);
#endif

    if (configBufferId == ConfigBufferId_HardwareConfig) {
        // reload is/ansi led settings
        Ledmap_InitLedLayout();
        EventVector_Set(EventVector_LedMapUpdateNeeded);
    }

    if (status != kStatus_Success) {
        Macros_ReportErrorPrintf(NULL, "UsbCommand_LaunchStorageTransfer failed with status %d", status);
        SetUsbTxBufferUint8(0, UsbStatusCode_LaunchStorageTransferTransferError);
        SetUsbTxBufferUint32(1, status);
    }
}
