#include "i2c.h"

i2c_master_handle_t I2cMasterHandle;
bool IsI2cTransferScheduled;
i2c_master_transfer_t masterTransfer;

void I2cAsyncWrite(uint8_t i2cAddress, uint8_t *data, size_t dataSize)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Write;
    masterTransfer.data = data;
    masterTransfer.dataSize = dataSize;
    IsI2cTransferScheduled = I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer) == kStatus_Success;
}

void I2cAsyncRead(uint8_t i2cAddress, uint8_t *data, size_t dataSize)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Read;
    masterTransfer.data = data;
    masterTransfer.dataSize = dataSize;
    IsI2cTransferScheduled = I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer) == kStatus_Success;
}
