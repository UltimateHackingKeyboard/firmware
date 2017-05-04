#include "fsl_i2c.h"
#include "bridge_protocol_scheduler.h"
#include "slot.h"
#include "main.h"
#include "slave_drivers/bridge_slave_led_driver.h"
#include "slave_drivers/bridge_slave_uhk_module.h"
#include "i2c.h"
#include "i2c_addresses.h"
#include "test_states.h"

uint8_t currentBridgeSlaveId = 0;

bridge_slave_t bridgeSlaves[] = {
    { .slaveHandler = BridgeSlaveUhkModuleHandler, .moduleId = 0 },
    { .slaveHandler = BridgeSlaveLedDriverHandler, .moduleId = 0 },
    { .slaveHandler = BridgeSlaveLedDriverHandler, .moduleId = 1 },
};

static void bridgeProtocolCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    if (TestStates.disableI2c) {
        return;
    }
    bridge_slave_t *bridgeSlave = bridgeSlaves + currentBridgeSlaveId;

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
    InitUhkModules();
    SetLeds(0xff);
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, bridgeProtocolCallback, NULL);

    // Kickstart the scheduler by triggering the first callback.
    bridgeSlaves[0].slaveHandler(bridgeSlaves[0].moduleId);
}
