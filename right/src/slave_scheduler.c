#include "fsl_i2c.h"
#include "slave_scheduler.h"
#include "slot.h"
#include "main.h"
#include "slave_drivers/slave_driver_led_driver.h"
#include "slave_drivers/slave_driver_uhk_module.h"
#include "i2c.h"
#include "i2c_addresses.h"
#include "test_states.h"

uint8_t currentBridgeSlaveId = 0;

uhk_slave_t slaves[] = {
    { .slaveHandler = UhkSlaveUhkModuleHandler, .moduleId = 0 },
    { .slaveHandler = UhkSlaveLedDriverHandler, .moduleId = 0 },
    { .slaveHandler = UhkSlaveLedDriverHandler, .moduleId = 1 },
};

static void bridgeProtocolCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    if (TestStates.disableI2c) {
        return;
    }
    uhk_slave_t *bridgeSlave = slaves + currentBridgeSlaveId;

    bridgeSlave->slaveHandler(bridgeSlave->moduleId);
    currentBridgeSlaveId++;

    if (currentBridgeSlaveId >= (sizeof(slaves) / sizeof(uhk_slave_t))) {
        currentBridgeSlaveId = 0;
    }
}

void InitSlaveScheduler()
{
    InitUhkModules();
    SetLeds(0xff);
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, bridgeProtocolCallback, NULL);

    // Kickstart the scheduler by triggering the first callback.
    slaves[0].slaveHandler(slaves[0].moduleId);
}
