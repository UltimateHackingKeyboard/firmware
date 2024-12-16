#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include "device.h"
#include "i2c_compatibility.h"
#include "timer.h"
#include "shared/slave_protocol.h"
#include "slave_scheduler.h"
#include "slave_drivers/uhk_module_driver.h"
#include "i2c.h"
#include "keyboard/i2c.h"

// Thread definitions

#define THREAD_STACK_SIZE 1000
#define THREAD_PRIORITY -1

static K_THREAD_STACK_DEFINE(stack_area, THREAD_STACK_SIZE);
static struct k_thread thread_data;

static bool masterTransferInProgress;
static i2c_master_transfer_t* masterTransfer;

const struct device *i2c0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));

status_t ZephyrI2c_MasterTransferNonBlocking(i2c_master_transfer_t *transfer) {
    if (masterTransferInProgress) {
        return kStatus_I2C_Busy;
    } else {
        masterTransfer = transfer;
        masterTransferInProgress = true;
        return kStatus_Success;
    }
}

status_t processMasterTransfer() {
    if (masterTransfer->direction == kI2C_Write) {
        status_t ret = i2c_write(i2c0_dev, masterTransfer->data, masterTransfer->dataSize, masterTransfer->slaveAddress);
        masterTransferInProgress = false;
        if (ret != 0) {
            return kStatus_Fail;
        }
        return kStatus_Success;
    }
    if (masterTransfer->direction == kI2C_Read && masterTransfer->dataSize == I2C_MESSAGE_MAX_TOTAL_LENGTH) {
        struct i2c_msg msg;
        msg.buf = masterTransfer->data;
        msg.len = 1;
        msg.flags = I2C_MSG_READ; // Unlike i2c_read(), don't use I2C_MSG_STOP
        int ret = i2c_transfer(i2c0_dev, &msg, 1, masterTransfer->slaveAddress);
        size_t msgLen = masterTransfer->data[0];

        if (ret != 0) {
            return kStatus_Fail;
        }

        ret = i2c_read(i2c0_dev, masterTransfer->data+1, msgLen+2, masterTransfer->slaveAddress);
        if (ret != 0) {
            return kStatus_Fail;
        }

        masterTransferInProgress = false;
        return kStatus_Success;
    }
    if (masterTransfer->direction == kI2C_Read) {
        int ret = i2c_read(i2c0_dev, masterTransfer->data, masterTransfer->dataSize, masterTransfer->slaveAddress);

        masterTransferInProgress = false;

        if (ret != 0) {
            return kStatus_Fail;
        }
        return kStatus_Success;
    }
    masterTransferInProgress = false;
    return kStatus_Fail;
}

void i2cPoller() {
    InitSlaveScheduler();

    while (true) {
        static status_t lastStatus = kStatus_Success;

        SlaveSchedulerCallback(lastStatus);

        if (masterTransfer->direction == kI2C_Write) {
            k_msleep(1);
        }

        if (masterTransferInProgress) {
            lastStatus = processMasterTransfer();
        }
    }
}

void InitZephyrI2c(void) {
    k_thread_create(
        &thread_data, stack_area,
        K_THREAD_STACK_SIZEOF(stack_area),
        i2cPoller,
        NULL, NULL, NULL,
        THREAD_PRIORITY, 0, K_NO_WAIT
    );

    k_thread_name_set(&thread_data, "i2c_poller");
}
