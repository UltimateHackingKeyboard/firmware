#include "fsl_i2c.h"
#include "i2c_addresses.h"

void I2cRead(I2C_Type *baseAddress, uint8_t i2cAddress, uint8_t *data, uint8_t size)
{
    i2c_master_transfer_t masterXfer;
    masterXfer.slaveAddress = i2cAddress;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = data;
    masterXfer.dataSize = size;
    masterXfer.flags = kI2C_TransferDefaultFlag;
    I2C_MasterTransferBlocking(baseAddress, &masterXfer);
}

void I2cWrite(I2C_Type *baseAddress, uint8_t i2cAddress, uint8_t *data, uint8_t size)
{
    i2c_master_transfer_t masterXfer;
    masterXfer.slaveAddress = i2cAddress;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = data;
    masterXfer.dataSize = size;
    masterXfer.flags = kI2C_TransferDefaultFlag;
    I2C_MasterTransferBlocking(baseAddress, &masterXfer);
}
