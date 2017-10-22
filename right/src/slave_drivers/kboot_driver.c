#include "slave_drivers/kboot_driver.h"
#include "slave_scheduler.h"
#include "i2c.h"

kboot_driver_state_t KbootDriverState;

static uint8_t rxBuffer[MAX_KBOOT_COMMAND_LENGTH];
static uint8_t resetCommand[] = {0x5a, 0xa4, 0x04, 0x00, 0x6f, 0x46, 0x0b, 0x00, 0x00, 0x00};
static uint8_t ackMessage[] = {0x5a, 0xa1};
static status_t tx(uint8_t *buffer, uint8_t length)
{
    return I2cAsyncWrite(KbootDriverState.i2cAddress, buffer, length);
}

static status_t rx(uint8_t length)
{
    return I2cAsyncRead(KbootDriverState.i2cAddress, rxBuffer, length);
}

void KbootSlaveDriver_Init(uint8_t kbootInstanceId)
{
}

status_t KbootSlaveDriver_Update(uint8_t kbootInstanceId)
{
    status_t status = kStatus_Uhk_IdleSlave;

    if (!KbootDriverState.isTransferScheduled) {
        return status;
    }

    switch (KbootDriverState.phase++) {
        case 0:
            status = tx(resetCommand, sizeof(resetCommand));
            break;
        case 1:
            status = rx(2);
            break;
        case 2:
            status = rx(18);
            break;
        case 3:
            status = tx(ackMessage, sizeof(ackMessage));
            KbootDriverState.isTransferScheduled = false;
    }

    return status;
}
