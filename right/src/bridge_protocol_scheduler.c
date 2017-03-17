#include "fsl_i2c.h"
#include "led_driver.h"
#include "bridge_protocol_scheduler.h"
#include "slot.h"
#include "main.h"

#define BUFFER_SIZE (LED_DRIVER_LED_COUNT + 1)

uint8_t ledsBuffer[BUFFER_SIZE] = {FRAME_REGISTER_PWM_FIRST};
uint8_t currentBridgeSlaveId = 0;

bridge_slave_t bridgeSlaves[] = {
    { .moduleId = 0, .type = BridgeSlaveType_UhkModule },
    { .moduleId = 0, .type = BridgeSlaveType_LedDriver },
};

bool BridgeSlaveLedDriverHandler(uint8_t ledDriverId) {
    I2cAsyncWrite(I2C_ADDRESS_LED_DRIVER_LEFT, ledsBuffer, BUFFER_SIZE);
    return true;
}

bool BridgeSlaveUhkModuleHandler(uint8_t uhkModuleId) {
    I2cAsyncRead(I2C_ADDRESS_LEFT_KEYBOARD_HALF, CurrentKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], LEFT_KEYBOARD_HALF_KEY_COUNT);
    return true;
}

static void bridgeProtocolCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    bridge_slave_t *bridgeSlave = bridgeSlaves + currentBridgeSlaveId;
    SetLeds(0xff);

    if (bridgeSlave->type == BridgeSlaveType_UhkModule) {
        BridgeSlaveUhkModuleHandler(bridgeSlave->moduleId);
    } else if (bridgeSlave->type == BridgeSlaveType_LedDriver) {
        BridgeSlaveLedDriverHandler(bridgeSlave->moduleId);
    }

    if (++currentBridgeSlaveId >= (sizeof(bridgeSlaves) / sizeof(bridge_slave_t))) {
        currentBridgeSlaveId = 0;
    }
}

void InitBridgeProtocolScheduler()
{
    SetLeds(0xff);
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, bridgeProtocolCallback, NULL);
    I2cAsyncWrite(I2C_ADDRESS_LED_DRIVER_LEFT, ledsBuffer, BUFFER_SIZE);
}

void SetLeds(uint8_t ledBrightness)
{
    memset(ledsBuffer+1, ledBrightness, LED_DRIVER_LED_COUNT);
}
