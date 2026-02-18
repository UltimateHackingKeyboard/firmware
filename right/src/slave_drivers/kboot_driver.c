#include "slave_drivers/kboot_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_scheduler.h"
#include "i2c.h"
#include "i2c_addresses.h"
#include "logger.h"
#include "timer.h"

kboot_driver_state_t KbootDriverState;

static uint8_t rxBuffer[KBOOT_PACKAGE_MAX_LENGTH];
static uint8_t pingCommand[] = {0x5a, 0xa6};
static uint8_t resetCommand[] = {0x5a, 0xa4, 0x04, 0x00, 0x6f, 0x46, 0x0b, 0x00, 0x00, 0x00};
static uint8_t ackMessage[] = {0x5a, 0xa1};

#define KBOOT_WAIT_AFTER_JUMP_MS 10
#define KBOOT_PING_TIMEOUT_MS    200

static status_t tx(uint8_t *buffer, uint8_t length)
{
    return I2cAsyncWrite(KbootDriverState.i2cAddress, buffer, length);
}

static status_t rx(uint8_t length)
{
    return I2cAsyncRead(KbootDriverState.i2cAddress, rxBuffer, length);
}

static uint32_t elapsedMs(void)
{
    return Timer_GetCurrentTime() - KbootDriverState.startTime;
}

void KbootSlaveDriver_Init(uint8_t kbootInstanceId)
{
}

slave_result_t KbootSlaveDriver_Update(uint8_t kbootInstanceId)
{
    slave_result_t res = { .status = kStatus_Uhk_IdleSlave, .hold = false };

    switch (KbootDriverState.command) {
        case KbootCommand_Idle:
            break;
        case KbootCommand_Ping:
            switch (KbootDriverState.phase) {
                case KbootPhase_SendPing:
                    res.status = tx(pingCommand, sizeof(pingCommand));
                    KbootDriverState.phase = KbootPhase_CheckPingStatus;
                    break;
                case KbootPhase_CheckPingStatus:
                    KbootDriverState.status = Slaves[SlaveId_KbootDriver].previousStatus;
                    KbootDriverState.phase = KbootDriverState.status == kStatus_Success
                        ? KbootPhase_ReceivePingResponse
                        : KbootPhase_SendPing;
                    res.status =  kStatus_Uhk_IdleCycle;
                    res.hold = true;
                    break;
                case KbootPhase_ReceivePingResponse:
                    res.status = rx(KBOOT_PACKAGE_LENGTH_PING_RESPONSE);
                    KbootDriverState.phase = KbootPhase_CheckPingResponseStatus;
                    break;
                case KbootPhase_CheckPingResponseStatus:
                    KbootDriverState.status = Slaves[SlaveId_KbootDriver].previousStatus;
                    if (KbootDriverState.status == kStatus_Success) {
                        KbootDriverState.command = KbootCommand_Idle;
                    } else {
                        KbootDriverState.phase = KbootPhase_SendPing;
                        res.status =  kStatus_Uhk_IdleCycle;
                        res.hold = true;
                    }
                    break;
                }
            break;
        case KbootCommand_Reset:
            switch (KbootDriverState.phase) {
                case KbootPhase_SendReset:
                    res.status = tx(resetCommand, sizeof(resetCommand));
                    KbootDriverState.phase = KbootPhase_ReceiveResetAck;
                    break;
                case KbootPhase_ReceiveResetAck:
                    res.status = rx(KBOOT_PACKAGE_LENGTH_ACK);
                    KbootDriverState.phase = KbootPhase_ReceiveResetGenericResponse;
                    break;
                case KbootPhase_ReceiveResetGenericResponse:
                    res.status = rx(KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                    KbootDriverState.phase = KbootPhase_CheckResetSendAck;
                    break;
                case KbootPhase_CheckResetSendAck:
                    res.status = tx(ackMessage, sizeof(ackMessage));
                    KbootDriverState.command = KbootCommand_Idle;
                    break;
            }
            break;
        case KbootCommand_Flash:
            switch (KbootDriverState.phase) {
                case KbootFlashPhase_JumpToBootloader:
                    LogU("Kboot: Jumping to bootloader\n");
                    UhkModuleStates[UhkModuleDriverId_RightModule].phase = UhkModulePhase_JumpToBootloader;
                    KbootDriverState.i2cAddress = I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER;
                    KbootDriverState.startTime = Timer_GetCurrentTime();
                    KbootDriverState.phase = KbootFlashPhase_WaitForBootloader;
                    break;
                case KbootFlashPhase_WaitForBootloader:
                    if (elapsedMs() < KBOOT_WAIT_AFTER_JUMP_MS) {
                        break;
                    }
                    LogU("Kboot: Wait done (%dms), pinging bootloader at 0x%02x\n",
                         KBOOT_WAIT_AFTER_JUMP_MS, KbootDriverState.i2cAddress);
                    KbootDriverState.phase = KbootFlashPhase_SendPing;
                    break;
                case KbootFlashPhase_SendPing:
                    res.status = tx(pingCommand, sizeof(pingCommand));
                    KbootDriverState.phase = KbootFlashPhase_CheckPingStatus;
                    break;
                case KbootFlashPhase_CheckPingStatus:
                    KbootDriverState.status = Slaves[SlaveId_KbootDriver].previousStatus;
                    if (KbootDriverState.status == kStatus_Success) {
                        KbootDriverState.phase = KbootFlashPhase_ReceivePingResponse;
                    } else if (elapsedMs() > KBOOT_PING_TIMEOUT_MS) {
                        LogU("Kboot: Ping timed out after %dms (last status=%d)\n",
                             KBOOT_PING_TIMEOUT_MS, KbootDriverState.status);
                        KbootDriverState.command = KbootCommand_Idle;
                        break;
                    } else {
                        KbootDriverState.phase = KbootFlashPhase_SendPing;
                    }
                    res.status = kStatus_Uhk_IdleCycle;
                    res.hold = true;
                    break;
                case KbootFlashPhase_ReceivePingResponse:
                    res.status = rx(KBOOT_PACKAGE_LENGTH_PING_RESPONSE);
                    KbootDriverState.phase = KbootFlashPhase_CheckPingResponseStatus;
                    break;
                case KbootFlashPhase_CheckPingResponseStatus:
                    KbootDriverState.status = Slaves[SlaveId_KbootDriver].previousStatus;
                    if (KbootDriverState.status == kStatus_Success) {
                        LogU("Kboot: Bootloader ping OK! (%dms after jump)\n", elapsedMs());
                        KbootDriverState.phase = KbootFlashPhase_SendReset;
                    } else if (elapsedMs() > KBOOT_PING_TIMEOUT_MS) {
                        LogU("Kboot: Ping response timed out after %dms (last status=%d)\n",
                             KBOOT_PING_TIMEOUT_MS, KbootDriverState.status);
                        KbootDriverState.command = KbootCommand_Idle;
                        break;
                    } else {
                        KbootDriverState.phase = KbootFlashPhase_SendPing;
                    }
                    res.status = kStatus_Uhk_IdleCycle;
                    res.hold = true;
                    break;
                case KbootFlashPhase_SendReset:
                    LogU("Kboot: Sending reset to return to firmware\n");
                    res.status = tx(resetCommand, sizeof(resetCommand));
                    KbootDriverState.phase = KbootFlashPhase_ReceiveResetAck;
                    break;
                case KbootFlashPhase_ReceiveResetAck:
                    res.status = rx(KBOOT_PACKAGE_LENGTH_ACK);
                    KbootDriverState.phase = KbootFlashPhase_ReceiveResetGenericResponse;
                    break;
                case KbootFlashPhase_ReceiveResetGenericResponse:
                    res.status = rx(KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                    KbootDriverState.phase = KbootFlashPhase_SendResetAck;
                    break;
                case KbootFlashPhase_SendResetAck:
                    res.status = tx(ackMessage, sizeof(ackMessage));
                    LogU("Kboot: Flash sequence complete (%dms total)\n", elapsedMs());
                    KbootDriverState.command = KbootCommand_Idle;
                    break;
            }
            break;
    }

    return res;
}
