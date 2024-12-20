#include "fsl_common.h"
#include "usb_commands/usb_command_get_slave_i2c_errors.h"
#include "usb_protocol_handler.h"
#include "slave_scheduler.h"
#include "i2c_error_logger.h"

void UsbCommand_GetSlaveI2cErrors(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer)
{
    uint8_t slaveId = GetUsbRxBufferUint8(1);

    if (!IS_VALID_SLAVE_ID(slaveId)) {
        SetUsbTxBufferUint8(0, UsbStatusCode_GetModuleProperty_InvalidSlaveId);
    }

    i2c_slave_error_counter_t *i2cSlaveErrorCounter =  I2cSlaveErrorCounters + slaveId;

    GenericHidInBuffer[1] = i2cSlaveErrorCounter->errorTypeCount;
    memcpy(GenericHidInBuffer + 2, i2cSlaveErrorCounter->errors, sizeof(i2c_error_count_t) * MAX_LOGGED_I2C_ERROR_TYPES_PER_SLAVE);
}
