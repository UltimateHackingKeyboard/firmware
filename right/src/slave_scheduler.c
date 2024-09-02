#include "slave_scheduler.h"
#include "slot.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "slave_drivers/touchpad_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "i2c.h"
#include "i2c_addresses.h"
#include "i2c_error_logger.h"
#include "macros/core.h"
#include "debug.h"

#ifdef __ZEPHYR__
    #include "keyboard/i2c.h"
#else
    #include "fsl_common.h"
    #include "fsl_i2c.h"
    #include "fsl_clock.h"
#include "slave_drivers/kboot_driver.h"
#endif

uint32_t I2cSlaveScheduler_Counter;

static uint8_t previousSlaveId;
static uint8_t currentSlaveId;

uhk_slave_t Slaves[SLAVE_COUNT] = {
    {
        .init = UhkModuleSlaveDriver_Init,
        .update = UhkModuleSlaveDriver_Update,
        .disconnect = UhkModuleSlaveDriver_Disconnect,
        .perDriverId = UhkModuleDriverId_LeftKeyboardHalf,
    },
    {
        .init = UhkModuleSlaveDriver_Init,
        .update = UhkModuleSlaveDriver_Update,
        .disconnect = UhkModuleSlaveDriver_Disconnect,
        .perDriverId = UhkModuleDriverId_LeftModule,
    },
    {
        .init = UhkModuleSlaveDriver_Init,
        .update = UhkModuleSlaveDriver_Update,
        .disconnect = UhkModuleSlaveDriver_Disconnect,
        .perDriverId = UhkModuleDriverId_RightModule,
    },
    {
        .init = TouchpadDriver_Init,
        .update = TouchpadDriver_Update,
        .disconnect = TouchpadDriver_Disconnect,
        .perDriverId = TouchpadDriverId_Singleton,
    },
    {
        .init = LedSlaveDriver_Init,
        .update = LedSlaveDriver_Update,
        .perDriverId = LedDriverId_Right,
    },
    {
        .init = LedSlaveDriver_Init,
        .update = LedSlaveDriver_Update,
        .perDriverId = LedDriverId_Left,
    },
    {
        .init = LedSlaveDriver_Init,
        .update = LedSlaveDriver_Update,
        .perDriverId = LedDriverId_ModuleLeft,
    },
#ifndef __ZEPHYR__
    {
        .init = KbootSlaveDriver_Init,
        .update = KbootSlaveDriver_Update,
        .perDriverId = KbootDriverId_Singleton,
    },
#endif
};

static uint8_t getNextSlaveId(uint8_t slaveId)
{
#ifndef __ZEPHYR__
    slaveId++;
    if (slaveId >= SLAVE_COUNT) {
        slaveId = 0;
    }
    return slaveId;
#elif DEVICE_IS_UHK80_LEFT
    return SlaveId_LeftModule;
#elif DEVICE_IS_UHK80_RIGHT
    return slaveId == SlaveId_RightModule ? SlaveId_RightTouchpad : SlaveId_RightModule;
    // return SlaveId_RightTouchpad;
#else
    return slaveId;
#endif
}

static void slaveSchedulerCallback(I2C_Type *base, i2c_master_handle_t *handle, status_t previousStatus, void *userData)
{
    bool isFirstCycle = true;
    bool isTransferScheduled = false;
    I2cSlaveScheduler_Counter++;

    do {
        uhk_slave_t *currentSlave = Slaves + currentSlaveId;

        if (isFirstCycle) {
            uhk_slave_t *previousSlave = Slaves + previousSlaveId;
            previousSlave->previousStatus = previousStatus;
            if (IS_STATUS_I2C_ERROR(previousStatus)) {
                LogI2cError(previousSlaveId, previousStatus);
            }

            bool wasPreviousSlaveConnected = previousSlave->isConnected;
            previousSlave->isConnected = previousStatus == kStatus_Success;
            if (wasPreviousSlaveConnected && !previousSlave->isConnected && previousSlave->disconnect) {
                previousSlave->disconnect(previousSlave->perDriverId);
            }

            isFirstCycle = false;
        }

        if (!currentSlave->isConnected) {
            currentSlave->init(currentSlave->perDriverId);
        }

        slave_result_t res = currentSlave->update(currentSlave->perDriverId);
        status_t currentStatus = res.status;
        if (IS_STATUS_I2C_ERROR(currentStatus)) {
            LogI2cError(currentSlaveId, currentStatus);
        }

        isTransferScheduled = currentStatus != kStatus_Uhk_IdleSlave && currentStatus != kStatus_Uhk_IdleCycle;

        if (!res.hold || !currentSlave->isConnected) {
            previousSlaveId = currentSlaveId;
            currentSlaveId = getNextSlaveId(currentSlaveId);
        } else {
            previousSlaveId = currentSlaveId;
        }

    } while (!isTransferScheduled);
}

void SlaveSchedulerCallback(status_t status)
{
    slaveSchedulerCallback(NULL, NULL, status, NULL);
}

void InitSlaveScheduler(void)
{
    previousSlaveId = 0;
    currentSlaveId = 0;

    for (uint8_t i=0; i<SLAVE_COUNT; i++) {
        uhk_slave_t *currentSlave = Slaves + i;
        if (currentSlave->isConnected && currentSlave->disconnect) {
            currentSlave->disconnect(currentSlave->perDriverId);
        }
        currentSlave->isConnected = false;
    }

#ifndef __ZEPHYR__
    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, slaveSchedulerCallback, NULL);

    // Kickstart the scheduler by triggering the first transfer.
    slaveSchedulerCallback(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, kStatus_Fail, NULL);
#endif
}
