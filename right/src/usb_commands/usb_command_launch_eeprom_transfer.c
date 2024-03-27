#include "usb_commands/usb_command_launch_eeprom_transfer.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "config_parser/config_globals.h"

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
#endif

void UsbCommand_LaunchEepromTransfer(void)
{
    eeprom_operation_t eepromOperation = GetUsbRxBufferUint8(1);
    config_buffer_id_t configBufferId = GetUsbRxBufferUint8(2);

    if (!IsEepromOperationValid(eepromOperation)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_LaunchEepromTransfer_InvalidEepromOperation);
    }

    if (!IsConfigBufferIdValid(configBufferId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_LaunchEepromTransfer_InvalidConfigBufferId);
    }

#ifdef __ZEPHYR__
    status_t status = Flash_LaunchTransfer(eepromOperation, configBufferId, NULL);
#else
    status_t status = EEPROM_LaunchTransfer(eepromOperation, configBufferId, NULL);
#endif

    if (status != kStatus_Success) {
        SetUsbTxBufferUint8(0, UsbStatusCode_LaunchEepromTransfer_TransferError);
        SetUsbTxBufferUint32(1, status);
    }
}
