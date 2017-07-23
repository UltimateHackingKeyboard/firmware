#include "fsl_common.h"
#include "config/config_state.h"
#include "i2c_addresses.h"
#include "i2c.h"
#include "eeprom.h"

bool IsEepromBusy;
eeprom_transfer_t CurrentEepromTransfer;
status_t LastEepromTransferStatus;

static i2c_master_handle_t i2cHandle;
static i2c_master_transfer_t i2cTransfer;

static uint8_t *sourceBuffer;
static uint16_t sourceOffset;
static uint16_t sourceLength;

static status_t i2cAsyncWrite(uint8_t *data, size_t dataSize)
{
    i2cTransfer.slaveAddress = I2C_ADDRESS_EEPROM;
    i2cTransfer.direction = kI2C_Write;
    i2cTransfer.data = data;
    i2cTransfer.dataSize = dataSize;
    return I2C_MasterTransferNonBlocking(I2C_EEPROM_BUS_BASEADDR, &i2cHandle, &i2cTransfer);
}

static status_t i2cAsyncRead(uint8_t *data, size_t dataSize)
{
    i2cTransfer.slaveAddress = I2C_ADDRESS_EEPROM;
    i2cTransfer.direction = kI2C_Read;
    i2cTransfer.data = data;
    i2cTransfer.dataSize = dataSize;
    return I2C_MasterTransferNonBlocking(I2C_EEPROM_BUS_BASEADDR, &i2cHandle, &i2cTransfer);
}

static status_t writePage()
{
    static uint8_t buffer[EEPROM_BUFFER_SIZE];
    uint8_t pageLength = MIN(sourceLength - sourceOffset, EEPROM_PAGE_SIZE);
    buffer[0] = sourceOffset & 0xff;
    buffer[1] = sourceOffset >> 8;
    memcpy(buffer+EEPROM_ADDRESS_LENGTH, sourceBuffer+sourceOffset, pageLength);
    status_t status = i2cAsyncWrite(buffer, pageLength);
    sourceOffset += pageLength;
    return status;
}

static void i2cCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    LastEepromTransferStatus = status;
    if (status != kStatus_Success) {
        return;
    }

    bool isHardwareConfig = CurrentEepromTransfer == EepromTransfer_ReadHardwareConfiguration;
    switch (CurrentEepromTransfer) {
        case EepromTransfer_ReadHardwareConfiguration:
        case EepromTransfer_ReadUserConfiguration:
            LastEepromTransferStatus = i2cAsyncRead(
                isHardwareConfig ? HardwareConfigBuffer.buffer : UserConfigBuffer.buffer,
                isHardwareConfig ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE
            );
            IsEepromBusy = false;
            break;
        case EepromTransfer_WriteHardwareConfiguration:
        case EepromTransfer_WriteUserConfiguration:
            IsEepromBusy = sourceOffset < sourceLength;
            if (!IsEepromBusy) {
                return;
            }
            LastEepromTransferStatus = writePage();
            break;
        default:
            IsEepromBusy = false;
            break;
    }
}

void EEPROM_Init(void)
{
    I2C_MasterTransferCreateHandle(I2C_EEPROM_BUS_BASEADDR, &i2cHandle, i2cCallback, NULL);
}

status_t EEPROM_LaunchTransfer(eeprom_transfer_t transferType)
{
    if (IsEepromBusy) {
        return kStatus_I2C_Busy;
    }

    uint16_t eepromStartAddress;
    bool isHardwareConfig = CurrentEepromTransfer == EepromTransfer_ReadHardwareConfiguration ||
                            CurrentEepromTransfer == EepromTransfer_WriteHardwareConfiguration;
    CurrentEepromTransfer = transferType;

    switch (transferType) {
        case EepromTransfer_ReadHardwareConfiguration:
        case EepromTransfer_ReadUserConfiguration:
            eepromStartAddress = isHardwareConfig ?  0 : HARDWARE_CONFIG_SIZE;
            LastEepromTransferStatus = i2cAsyncWrite((uint8_t*)&eepromStartAddress, EEPROM_ADDRESS_LENGTH);
            break;
        case EepromTransfer_WriteHardwareConfiguration:
        case EepromTransfer_WriteUserConfiguration:
            sourceBuffer = isHardwareConfig ? HardwareConfigBuffer.buffer : UserConfigBuffer.buffer;
            sourceOffset = 0;
            sourceLength = isHardwareConfig ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;
            LastEepromTransferStatus = writePage();
            break;
    }
    IsEepromBusy = LastEepromTransferStatus == kStatus_Success;
    return LastEepromTransferStatus;
}
