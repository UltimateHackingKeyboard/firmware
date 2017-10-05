#include "fsl_common.h"
#include "config_parser/config_globals.h"
#include "i2c_addresses.h"
#include "i2c.h"
#include "eeprom.h"
#include "config_parser/config_globals.h"

bool IsEepromBusy;
static eeprom_operation_t CurrentEepromOperation;
static config_buffer_id_t CurrentConfigBufferId;
status_t LastEepromTransferStatus;
void (*SuccessCallback)();

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

static status_t writePage(void)
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

    switch (CurrentEepromOperation) {
        case EepromOperation_Read:
            if (isReadSent) {
                IsEepromBusy = false;
                if (SuccessCallback) {
                    SuccessCallback();
                }
                return;
            }
            LastEepromTransferStatus = i2cAsyncRead(
                ConfigBufferIdToConfigBuffer(CurrentConfigBufferId)->buffer,
                CurrentConfigBufferId == ConfigBufferId_HardwareConfig ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE
            );
            IsEepromBusy = true;
            isReadSent = true;
            break;
        case EepromOperation_Write:
            if (status == kStatus_Success) {
                sourceOffset += writeLength;
            }
            IsEepromBusy = sourceOffset < sourceLength;
            if (!IsEepromBusy) {
                if (SuccessCallback) {
                    SuccessCallback();
                }
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

status_t EEPROM_LaunchTransfer(eeprom_operation_t operation, config_buffer_id_t config_buffer_id, void (*successCallback))
{
    if (IsEepromBusy) {
        return kStatus_I2C_Busy;
    }

    CurrentEepromOperation = operation;
    CurrentConfigBufferId = config_buffer_id;

    SuccessCallback = successCallback;
    bool isHardwareConfig = CurrentConfigBufferId == ConfigBufferId_HardwareConfig;
    eepromStartAddress = isHardwareConfig ? 0 : HARDWARE_CONFIG_SIZE;
    addressBuffer[0] = eepromStartAddress >> 8;
    addressBuffer[1] = eepromStartAddress & 0xff;

    switch (CurrentEepromOperation) {
        case EepromOperation_Read:
            isReadSent = false;
            LastEepromTransferStatus = i2cAsyncWrite(addressBuffer, EEPROM_ADDRESS_LENGTH);
            break;
        case EepromOperation_Write:
            sourceBuffer = ConfigBufferIdToConfigBuffer(CurrentConfigBufferId)->buffer;
            sourceOffset = 0;
            sourceLength = isHardwareConfig ? HARDWARE_CONFIG_SIZE : USER_CONFIG_SIZE;
            LastEepromTransferStatus = writePage();
            break;
    }

    IsEepromBusy = LastEepromTransferStatus == kStatus_Success;
    return LastEepromTransferStatus;
}
