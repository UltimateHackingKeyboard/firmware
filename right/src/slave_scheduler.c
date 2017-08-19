#include "fsl_i2c.h"
#include "slave_scheduler.h"
#include "slot.h"
#include "main.h"
#include "slave_drivers/is31fl3731_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "i2c.h"
#include "i2c_addresses.h"

uint8_t previousSlaveId;
uint8_t currentSlaveId;
uint32_t BridgeCounter;

uhk_slave_t Slaves[] = {
    { .init = UhkModuleSlaveDriver_Init, .update = UhkModuleSlaveDriver_Update, .perDriverId = UhkModuleId_LeftKeyboardHalf },
    { .init = LedSlaveDriver_Init,       .update = LedSlaveDriver_Update,       .perDriverId = LedDriverId_Right            },
    { .init = LedSlaveDriver_Init,       .update = LedSlaveDriver_Update,       .perDriverId = LedDriverId_Left             },
};

static void bridgeProtocolCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t previousStatus, void *userData)
{
    bool isTransferScheduled = false;

    do {
        BridgeCounter++;

        uhk_slave_t *previousSlave = Slaves + previousSlaveId;
        uhk_slave_t *currentSlave = Slaves + currentSlaveId;

        previousSlave->isConnected = previousStatus == kStatus_Success;

        if (!currentSlave->isConnected) {
            currentSlave->init(currentSlave->perDriverId);
        }

        status_t currentStatus = currentSlave->update(currentSlave->perDriverId);
        isTransferScheduled = currentStatus == kStatus_Success;
        if (isTransferScheduled) {
            currentSlave->isConnected = true;
        }

        previousSlaveId = currentSlaveId;
        currentSlaveId++;

        if (currentSlaveId >= (sizeof(Slaves) / sizeof(uhk_slave_t))) {
            currentSlaveId = 0;
        }
    } while (!isTransferScheduled);
}

void InitSlaveScheduler()
{
    previousSlaveId = 0;
    currentSlaveId = 0;

    for (uint8_t i=0; i<sizeof(Slaves) / sizeof(uhk_slave_t); i++) {
        Slaves[i].isConnected = false;
    }

    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, bridgeProtocolCallback, NULL);

    // Kickstart the scheduler by triggering the first callback.
    Slaves[currentSlaveId].update(Slaves[currentSlaveId].perDriverId);
}
