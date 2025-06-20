#include "i2c.h"
#include "crc16.h"
#include "stubs.h"

#ifdef __ZEPHYR__
#include "keyboard/i2c.h"
#endif

i2c_master_handle_t I2cMasterHandle;
i2c_master_transfer_t masterTransfer;

status_t I2cAsyncWrite(uint8_t i2cAddress, uint8_t *data, size_t dataSize)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Write;
    masterTransfer.data = data;
    masterTransfer.dataSize = dataSize;
#ifdef __ZEPHYR__
    return ZephyrI2c_MasterTransferNonBlocking(&masterTransfer);
#else
    I2cMasterHandle.userData = NULL;
    return I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
#endif
}

status_t I2cAsyncWriteMessage(uint8_t i2cAddress, i2c_message_t *message)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Write;
    masterTransfer.data = (uint8_t*)message;
    masterTransfer.dataSize = I2C_MESSAGE_HEADER_LENGTH + message->length;
    CRC16_UpdateMessageChecksum(message);
#ifdef __ZEPHYR__
    return ZephyrI2c_MasterTransferNonBlocking(&masterTransfer);
#else
    I2cMasterHandle.userData = NULL;
    return I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
#endif
}

status_t I2cAsyncRead(uint8_t i2cAddress, uint8_t *data, size_t dataSize)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Read;
    masterTransfer.data = data;
    masterTransfer.dataSize = dataSize;
#ifdef __ZEPHYR__
    return ZephyrI2c_MasterTransferNonBlocking(&masterTransfer);
#else
    I2cMasterHandle.userData = NULL;
    return I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
#endif
}

#ifndef __ZEPHYR__
static void setVariableDataSize(i2c_master_handle_t *handle, uint8_t data)
{
    handle->userData = NULL; // only to be called at the first read byte
    handle->transfer.dataSize = 2 + data;  // 2 byte hash length + payload length
}

extern void I2C0_DriverIRQHandler(void);
volatile uint32_t I2C_Watchdog = 0;
void I2C0_IRQHandler(void)
{
    I2C_Watchdog++;
    I2C0_DriverIRQHandler();
}
#endif

status_t I2cAsyncReadMessage(uint8_t i2cAddress, i2c_message_t *message)
{
    masterTransfer.slaveAddress = i2cAddress;
    masterTransfer.direction = kI2C_Read;
    masterTransfer.data = (uint8_t*)message;
    masterTransfer.dataSize = I2C_MESSAGE_MAX_TOTAL_LENGTH;
#ifdef __ZEPHYR__
    return ZephyrI2c_MasterTransferNonBlocking(&masterTransfer);
#else
    I2cMasterHandle.userData = (void*)&setVariableDataSize;
    return I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, &masterTransfer);
#endif
}
