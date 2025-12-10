#include "uart_modules.h"

/*
#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY -1

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

void i2cPoller() {
    InitSlaveScheduler();

    while (true) {
        static status_t lastStatus = kStatus_Success;

        SlaveSchedulerCallback(lastStatus);

        if (masterTransfer->direction == kI2C_Write) {
            k_msleep(1);
        }

        if (masterTransferInProgress) {
            if (MergeSensor_HalvesAreMerged != MergeSensorState_Joined) {
                lastStatus = processMasterTransfer();
            } else {
                k_msleep(10);
                lastStatus = kStatus_Fail;
            }
        }
    }
}

void InitZephyrI2c(void) {
    if (PinWiringConfig->device_i2c_modules == NULL) {
        i2c0_dev = NULL;
        return;
    }

    i2c0_dev = PinWiringConfig->device_i2c_modules->device;

    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        i2cPoller,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );

    k_thread_name_set(&thread_data, "i2c_poller");
}

void InitUartModules(void)
{
    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        i2cPoller,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );

    k_thread_name_set(&thread_data, "module_poller");
}
*/
