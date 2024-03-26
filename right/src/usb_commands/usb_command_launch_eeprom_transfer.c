#include "fsl_common.h"
#include "usb_commands/usb_command_launch_eeprom_transfer.h"
#include "usb_protocol_handler.h"
#include "eeprom.h"
#include "config_parser/config_globals.h"

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
    status_t status = Storage_LaunchTransfer(eepromOperation, configBufferId, NULL);
#else
    status_t status = EEPROM_LaunchTransfer(eepromOperation, configBufferId, NULL);
#endif

    if (status != kStatus_Success) {
        SetUsbTxBufferUint8(0, UsbStatusCode_LaunchEepromTransfer_TransferError);
        SetUsbTxBufferUint32(1, status);
    }
}
