#include "fsl_common.h"
#include "config_parser/config_state.h"
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
static uint16_t eepromStartAddress;
static uint16_t sourceLength;
static uint8_t writeLength;
static bool isReadSent;
static uint8_t addressBuffer[EEPROM_ADDRESS_LENGTH];

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
    uint16_t targetEepromOffset = sourceOffset + eepromStartAddress;
    buffer[0] = targetEepromOffset >> 8;
    buffer[1] = targetEepromOffset & 0xff;
    writeLength = MIN(sourceLength - sourceOffset, EEPROM_PAGE_SIZE);
    memcpy(buffer+EEPROM_ADDRESS_LENGTH, sourceBuffer+sourceOffset, writeLength);
    status_t status = i2cAsyncWrite(buffer, writeLength+EEPROM_ADDRESS_LENGTH);
    return status;
}

static void i2cCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    LastEepromTransferStatus = status;

    bool isHardwareConfig = CurrentEepromTransfer == EepromTransfer_ReadHardwareConfiguration;
    switch (CurrentEepromTransfer) {
        case EepromTransfer_ReadHardwareConfiguration:
        case EepromTransfer_ReadUserConfiguration:
            if (isReadSent) {
                IsEepromBusy = false;
                return;
            }
            LastEepromTransferStatus = i2cAsyncRead(
                isHardwareConfig ? HardwareConfigBuffer.buffer : UserConfigBuffer.buffer,
                isHardwareConfig ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE
            );
            IsEepromBusy = true;
            isReadSent = true;
            break;
        case EepromTransfer_WriteHardwareConfiguration:
        case EepromTransfer_WriteUserConfiguration:
            if (status == kStatus_Success) {
                sourceOffset += writeLength;
            }
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

    CurrentEepromTransfer = transferType;
    bool isHardwareConfig = CurrentEepromTransfer == EepromTransfer_ReadHardwareConfiguration ||
                            CurrentEepromTransfer == EepromTransfer_WriteHardwareConfiguration;
    eepromStartAddress = isHardwareConfig ? 0 : HARDWARE_CONFIG_SIZE;
    addressBuffer[0] = eepromStartAddress >> 8;
    addressBuffer[1] = eepromStartAddress & 0xff;

    switch (transferType) {
        case EepromTransfer_ReadHardwareConfiguration:
        case EepromTransfer_ReadUserConfiguration:
            isReadSent = false;
            LastEepromTransferStatus = i2cAsyncWrite(addressBuffer, EEPROM_ADDRESS_LENGTH);
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
