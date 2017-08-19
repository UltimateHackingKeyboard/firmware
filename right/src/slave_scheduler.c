#include "fsl_i2c.h"
#include "slave_scheduler.h"
#include "slot.h"
#include "main.h"
#include "slave_drivers/is31fl3731_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "i2c.h"
#include "i2c_addresses.h"

uint8_t previousSlaveId = 0;
uint8_t currentSlaveId = 0;
uint32_t BridgeCounter;

uhk_slave_t Slaves[] = {
    { .init = UhkModuleSlaveDriver_Init, .update = UhkModuleSlaveDriver_Update, .perDriverId = UhkModuleId_LeftKeyboardHalf },
    { .init = LedSlaveDriver_Init,       .update = LedSlaveDriver_Update,       .perDriverId = LedDriverId_Right            },
    { .init = LedSlaveDriver_Init,       .update = LedSlaveDriver_Update,       .perDriverId = LedDriverId_Left             },
};

static void bridgeProtocolCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    IsI2cTransferScheduled = false;

    do {
        BridgeCounter++;

        uhk_slave_t *previousSlave = Slaves + previousSlaveId;
        uhk_slave_t *currentSlave = Slaves + currentSlaveId;

        previousSlave->isConnected = status == kStatus_Success;

        if (!currentSlave->isConnected) {
            currentSlave->init(currentSlave->perDriverId);
        }

        currentSlave->update(currentSlave->perDriverId);
        if (IsI2cTransferScheduled) {
            currentSlave->isConnected = true;
        }

        previousSlaveId = currentSlaveId;
        currentSlaveId++;

        if (currentSlaveId >= (sizeof(Slaves) / sizeof(uhk_slave_t))) {
            currentSlaveId = 0;
        }
    } while (!IsI2cTransferScheduled);
}

static void initSlaveDrivers()
{
    for (uint8_t i=0; i<sizeof(Slaves) / sizeof(uhk_slave_t); i++) {
        Slaves[i].init(Slaves[i].perDriverId);
    }
}

void InitSlaveScheduler()
{
    initSlaveDrivers();
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, bridgeProtocolCallback, NULL);

    // Kickstart the scheduler by triggering the first callback.
    Slaves[currentSlaveId].update(Slaves[currentSlaveId].perDriverId);
}
