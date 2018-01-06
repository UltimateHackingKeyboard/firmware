#include "slave_drivers/kboot_driver.h"
#include "slave_scheduler.h"
#include "i2c.h"

kboot_driver_state_t KbootDriverState;

static uint8_t rxBuffer[KBOOT_PACKAGE_MAX_LENGTH];
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
                case KbootPhase_SendPing:
                    status = tx(pingCommand, sizeof(pingCommand));
                    KbootDriverState.phase = KbootPhase_CheckPingStatus;
                    break;
                case KbootPhase_CheckPingStatus:
                    KbootDriverState.status = Slaves[SlaveId_KbootDriver].previousStatus;
                    KbootDriverState.phase = KbootDriverState.status == kStatus_Success
                        ? KbootPhase_ReceivePingResponse
                        : KbootPhase_SendPing;
                    return kStatus_Uhk_IdleCycle;
                case KbootPhase_ReceivePingResponse:
                    status = rx(KBOOT_PACKAGE_LENGTH_PING_RESPONSE);
                    KbootDriverState.phase = KbootPhase_CheckPingResponseStatus;
                    break;
                case KbootPhase_CheckPingResponseStatus:
                    KbootDriverState.status = Slaves[SlaveId_KbootDriver].previousStatus;
                    if (KbootDriverState.status == kStatus_Success) {
                        KbootDriverState.commandType = KbootCommand_Idle;
                    } else {
                        KbootDriverState.phase = KbootPhase_SendPing;
                        return kStatus_Uhk_IdleCycle;
                    }
                }
            break;
        case KbootCommand_Reset:
            switch (KbootDriverState.phase) {
                case KbootPhase_SendReset:
                    status = tx(resetCommand, sizeof(resetCommand));
                    KbootDriverState.phase = KbootPhase_ReceiveResetAck;
                    break;
                case KbootPhase_ReceiveResetAck:
                    status = rx(KBOOT_PACKAGE_LENGTH_ACK);
                    KbootDriverState.phase = KbootPhase_ReceiveResetGenericResponse;
                    break;
                case KbootPhase_ReceiveResetGenericResponse:
                    status = rx(KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                    KbootDriverState.phase = KbootPhase_CheckResetSendAck;
                    break;
                case KbootPhase_CheckResetSendAck:
                    status = tx(ackMessage, sizeof(ackMessage));
                    KbootDriverState.commandType = KbootCommand_Idle;
                    break;
            }
            break;
    }

    return status;
}
