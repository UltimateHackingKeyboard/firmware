#include "fsl_i2c.h"
#include "slave_scheduler.h"
#include "slot.h"
#include "main.h"
#include "slave_drivers/slave_driver_led_driver.h"
#include "slave_drivers/slave_driver_uhk_module.h"
#include "i2c.h"
#include "i2c_addresses.h"
#include "test_states.h"

uint8_t currentSlaveId = 0;

uhk_slave_t slaves[] = {
    { .initializer = UhkModuleSlaveDriver_Init, .updater = UhkModuleSlaveDriver_Update, .perDriverId = 0 },
    { .initializer = LedSlaveDriver_Init,       .updater = LedSlaveDriver_Update,       .perDriverId = 0 },
    { .initializer = LedSlaveDriver_Init,       .updater = LedSlaveDriver_Update,       .perDriverId = 1 },
};

static void bridgeProtocolCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    IsI2cTransferScheduled = false;

    do {
        if (TestStates.disableI2c) {
            return;
        }
        uhk_slave_t *slave = slaves + currentSlaveId;

        slave->isConnected = status == kStatus_Success;
        if (!slave->isConnected) {
            slave->initializer(slave->perDriverId);
        }

        slave->updater(slave->perDriverId);
        currentSlaveId++;

        if (currentSlaveId >= (sizeof(slaves) / sizeof(uhk_slave_t))) {
            currentSlaveId = 0;
        }
    } while (!IsI2cTransferScheduled);
}

static void initSlaveDrivers()
{
    for (uint8_t i=0; i<sizeof(slaves) / sizeof(uhk_slave_t); i++) {
        slaves[i].initializer(slaves[i].perDriverId);
    }
}

void InitSlaveScheduler()
{
    initSlaveDrivers();
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, bridgeProtocolCallback, NULL);

    // Kickstart the scheduler by triggering the first callback.
    slaves[0].updater(slaves[0].perDriverId);
}
