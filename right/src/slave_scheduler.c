#include "fsl_i2c.h"
#include "slave_scheduler.h"
#include "slot.h"
#include "slave_drivers/is31fl3xxx_driver.h"
#include "slave_drivers/touchpad_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_drivers/kboot_driver.h"
#include "i2c.h"
#include "i2c_addresses.h"
#include "config.h"
#include "i2c_error_logger.h"
#include "macros.h"
#include "debug.h"

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
    {
        .init = KbootSlaveDriver_Init,
        .update = KbootSlaveDriver_Update,
        .perDriverId = KbootDriverId_Singleton,
    },
};

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
            bool isPreviousSlaveConnected = previousStatus == kStatus_Success;

            if (previousSlaveId == SlaveId_RightTouchpad) {
                // Touchpad automatically enters sleep mode. Protocol for
                // forced communication includes one NACK exchange.
                static uint8_t nackCount = 0;
                if (isPreviousSlaveConnected) {
                    nackCount = 0;
                }
                if (nackCount < 2 && !isPreviousSlaveConnected) {
                    nackCount++;
                    isPreviousSlaveConnected = true;
                }
            }

            previousSlave->isConnected = isPreviousSlaveConnected;
            if (wasPreviousSlaveConnected && !isPreviousSlaveConnected && previousSlave->disconnect) {
                previousSlave->disconnect(previousSlave->perDriverId);
            }

            isFirstCycle = false;
        }

        if (!currentSlave->isConnected) {
            currentSlave->init(currentSlave->perDriverId);
        }

        status_t currentStatus = currentSlave->update(currentSlave->perDriverId);
        if (IS_STATUS_I2C_ERROR(currentStatus)) {
            LogI2cError(currentSlaveId, currentStatus);
        }
        isTransferScheduled = currentStatus != kStatus_Uhk_IdleSlave && currentStatus != kStatus_Uhk_IdleCycle;

        if (currentStatus != kStatus_Uhk_IdleCycle) {
            previousSlaveId = currentSlaveId++;
            if (currentSlaveId >= SLAVE_COUNT) {
                currentSlaveId = 0;
            }
        }

    } while (!isTransferScheduled);
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

    I2C_MasterTransferCreateHandle(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, slaveSchedulerCallback, NULL);

    // Kickstart the scheduler by triggering the first transfer.
    slaveSchedulerCallback(I2C_MAIN_BUS_BASEADDR, &I2cMasterHandle, kStatus_Fail, NULL);
}
