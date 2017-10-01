#include "i2c.h"
#include "crc16.h"

i2c_master_handle_t I2cMasterHandle;
i2c_master_transfer_t masterTransfer;

status_t I2cAsyncWrite(uint8_t i2cAddress, uint8_t *data, size_t dataSize)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Write;
    masterTransfer.data = data;
    masterTransfer.dataSize = dataSize;
    I2cMasterHandle.userData = NULL;
    return I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
}

status_t I2cAsyncWriteMessage(uint8_t i2cAddress, i2c_message_t *message)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Write;
    masterTransfer.data = (uint8_t*)message;
    masterTransfer.dataSize = I2C_MESSAGE_HEADER_LENGTH + message->length;
    I2cMasterHandle.userData = NULL;
    CRC16_UpdateMessageChecksum(message);
    return I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
}

status_t I2cAsyncRead(uint8_t i2cAddress, uint8_t *data, size_t dataSize)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Read;
    masterTransfer.data = data;
    masterTransfer.dataSize = dataSize;
    I2cMasterHandle.userData = NULL;
    return I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
}

status_t I2cAsyncReadMessage(uint8_t i2cAddress, i2c_message_t *message)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Read;
    masterTransfer.data = (uint8_t*)message;
    masterTransfer.dataSize = I2C_MESSAGE_MAX_TOTAL_LENGTH;
    I2cMasterHandle.userData = (void*)1;
    return I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
}
