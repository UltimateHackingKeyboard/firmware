#include "fsl_i2c.h"
#include "led_driver.h"
#include "bridge_protocol_scheduler.h"
#include "slot.h"
#include "main.h"

#define BUFFER_SIZE (LED_DRIVER_LED_COUNT + 1)

i2c_master_handle_t masterHandle;
i2c_master_config_t masterConfig;
uint8_t ledsBuffer[BUFFER_SIZE] = {FRAME_REGISTER_PWM_FIRST};
i2c_master_transfer_t masterXfer;
uint8_t currentBridgeSlaveId = 0;

bridge_slave_t bridgeSlaves[] = {
        { .i2cAddress = I2C_ADDRESS_LEFT_KEYBOARD_HALF, .type = BridgeSlaveType_UhkModule },
        { .i2cAddress = I2C_ADDRESS_LED_DRIVER_LEFT,    .type = BridgeSlaveType_LedDriver },
//        { .i2cAddress = I2C_ADDRESS_LED_DRIVER_RIGHT,   .type = BridgeSlaveType_LedDriver }
};

void i2cAsyncWrite(uint8_t i2cAddress, uint8_t *volatile data, volatile size_t dataSize)
{
    masterXfer.slaveAddress = i2cAddress;
    masterXfer.direction = kI2C_Write;
    masterXfer.data = data;
    masterXfer.dataSize = dataSize;
    I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &masterHandle, &masterXfer);
}

void i2cAsyncRead(uint8_t i2cAddress, uint8_t *volatile data, volatile size_t dataSize)
{
    masterXfer.slaveAddress = i2cAddress;
    masterXfer.direction = kI2C_Read;
    masterXfer.data = data;
    masterXfer.dataSize = dataSize;
    I2C_MasterTransferNonBlocking(I2C_MAIN_BUS_BASEADDR, &masterHandle, &masterXfer);
}

static void bridgeProtocolCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    bridge_slave_t *bridgeSlave = bridgeSlaves + currentBridgeSlaveId;
    SetLeds(0xff);

    if (bridgeSlave->type == BridgeSlaveType_UhkModule) {
        i2cAsyncRead(bridgeSlave->i2cAddress, CurrentKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], LEFT_KEYBOARD_HALF_KEY_COUNT);
    } else if (bridgeSlave->type == BridgeSlaveType_LedDriver) {
        i2cAsyncWrite(bridgeSlave->i2cAddress, ledsBuffer, BUFFER_SIZE);
    }

    if (++currentBridgeSlaveId >= (sizeof(bridgeSlaves) / sizeof(bridge_slave_t))) {
        currentBridgeSlaveId = 0;
    }
}

void InitBridgeProtocolScheduler()
{
    SetLeds(0xff);
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &masterHandle, bridgeProtocolCallback, NULL);
    i2cAsyncWrite(I2C_ADDRESS_LED_DRIVER_LEFT, ledsBuffer, BUFFER_SIZE);
}

void SetLeds(uint8_t ledBrightness)
{
    memset(ledsBuffer+1, ledBrightness, LED_DRIVER_LED_COUNT);
}
