#include "fsl_i2c.h"
#include "bridge_protocol_scheduler.h"
#include "slot.h"
#include "main.h"
#include "bridge_slave_led_driver_handler.h"
#include "i2c.h"
#include "i2c_addresses.h"

uint8_t currentBridgeSlaveId = 0;

bool BridgeSlaveUhkModuleHandler(uint8_t uhkModuleId) {
    I2cAsyncRead(I2C_ADDRESS_LEFT_KEYBOARD_HALF, CurrentKeyStates[SLOT_ID_LEFT_KEYBOARD_HALF], LEFT_KEYBOARD_HALF_KEY_COUNT);
    return true;
}

bridge_slave_t bridgeSlaves[] = {
    { .slaveHandler = BridgeSlaveUhkModuleHandler, .moduleId = 0 },
    { .slaveHandler = BridgeSlaveLedDriverHandler, .moduleId = 0 },
    { .slaveHandler = BridgeSlaveLedDriverHandler, .moduleId = 1 },
};

static void bridgeProtocolCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    bridge_slave_t *bridgeSlave = bridgeSlaves + currentBridgeSlaveId;
    SetLeds(0xff);

    bool isFinished = bridgeSlave->slaveHandler(bridgeSlave->moduleId);
    if (isFinished) {
        currentBridgeSlaveId++;

        if (currentBridgeSlaveId >= (sizeof(bridgeSlaves) / sizeof(bridge_slave_t))) {
            currentBridgeSlaveId = 0;
        }
    }
}

void InitBridgeProtocolScheduler()
{
    SetLeds(0xff);
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, bridgeProtocolCallback, NULL);
    bridgeSlaves[0].slaveHandler(bridgeSlaves[0].moduleId);
}
