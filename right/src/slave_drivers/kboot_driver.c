#include "slave_drivers/kboot_driver.h"
#include "slave_scheduler.h"
#include "i2c.h"

kboot_driver_state_t KbootDriverState;

static uint8_t rxBuffer[MAX_KBOOT_COMMAND_LENGTH];
static uint8_t pingCommand[] = {0x5a, 0xa6};
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

    switch (KbootDriverState.commandType) {
        case KbootCommand_Idle:
            break;
        case KbootCommand_Ping:
            switch (KbootDriverState.phase) {
                case 0:
                    status = tx(pingCommand, sizeof(pingCommand));
                    KbootDriverState.phase++;
                    break;
                case 1:
                    KbootDriverState.status = Slaves[SlaveId_KbootDriver].previousStatus;
                    KbootDriverState.phase = KbootDriverState.status == kStatus_Success ? 2 : 0;
                    return kStatus_Uhk_NoTransfer;
                case 2:
                    status = rx(10);
                    KbootDriverState.phase++;
                    break;
                case 3:
                    KbootDriverState.status = Slaves[SlaveId_KbootDriver].previousStatus;
                    if (KbootDriverState.status == kStatus_Success) {
                        KbootDriverState.commandType = KbootCommand_Idle;
                    } else {
                        KbootDriverState.phase = 0;
                        return kStatus_Uhk_NoTransfer;
                    }
                }
            break;
        case KbootCommand_Reset:
            switch (KbootDriverState.phase) {
                case 0:
                    status = tx(resetCommand, sizeof(resetCommand));
                    KbootDriverState.phase++;
                    break;
                case 1:
                    status = rx(2);
                    KbootDriverState.phase++;
                    break;
                case 2:
                    status = rx(18);
                    KbootDriverState.phase++;
                    break;
                case 3:
                    status = tx(ackMessage, sizeof(ackMessage));
                    KbootDriverState.commandType = KbootCommand_Idle;
                    break;
            }
            break;
    }

    return status;
}
