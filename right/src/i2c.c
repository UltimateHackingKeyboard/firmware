#include "i2c.h"

i2c_master_handle_t I2cMasterHandle;
i2c_master_transfer_t masterTransfer;

status_t I2cAsyncWrite(uint8_t i2cAddress, uint8_t *data, size_t dataSize)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Write;
    masterTransfer.data = data;
    masterTransfer.dataSize = dataSize;
    status_t status = I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
    return status;
}

status_t I2cAsyncRead(uint8_t i2cAddress, uint8_t *data, size_t dataSize)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Read;
    masterTransfer.data = data;
    masterTransfer.dataSize = dataSize;
    status_t status = I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
    return status;
}
