#include "slave_drivers/kboot_driver.h"
#include "slave_drivers/uhk_module_driver.h"
#include "slave_scheduler.h"
#include "i2c.h"
#include "i2c_addresses.h"
#include "logger.h"
#include "timer.h"
#include "crc16.h"
#include "config_parser/config_globals.h"
#include "module_flash.h"
#include <string.h>
#include <stdio.h>

kboot_driver_state_t KbootDriverState;

static uint8_t rxBuffer[KBOOT_PACKAGE_MAX_LENGTH + 6]; // max framing packet
static uint8_t txBuffer[6 + KBOOT_PACKAGE_MAX_LENGTH];
static uint8_t pingCommand[] = {0x5a, 0xa6};
static uint8_t ackMessage[] = {0x5a, 0xa1};
static uint32_t pingAttemptCount;

static bool shouldLogPing(void)
{
    // Log every attempt for the first 5, then every 20th.
    return pingAttemptCount <= 5 || pingAttemptCount % 20 == 0;
}

#define KBOOT_WAIT_AFTER_JUMP_MS 10
#define KBOOT_PING_TIMEOUT_MS    10000
#define KBOOT_DEFAULT_I2C_ADDRESS 0x10
#define KBOOT_DATA_CHUNK_SIZE    32
#define KBOOT_ERASE_TIMEOUT_MS   30000
#define KBOOT_CMD_TIMEOUT_MS     5000
#define KBOOT_PROGRESS_LOG_BYTES 8192

// ---------------------------------------------------------------------------
// I2C helpers
// ---------------------------------------------------------------------------

static status_t i2cTx(uint8_t *buffer, uint8_t length)
{
    return I2cAsyncWrite(KbootDriverState.i2cAddress, buffer, length);
}

static status_t i2cRx(uint8_t length)
{
    return I2cAsyncRead(KbootDriverState.i2cAddress, rxBuffer, length);
}

static status_t previousI2cStatus(void)
{
    return Slaves[SlaveId_KbootDriver].previousStatus;
}

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------

static uint32_t elapsedMs(void)
{
    return Timer_GetCurrentTime() - KbootDriverState.startTime;
}

static uint32_t cmdElapsedMs(void)
{
    return Timer_GetCurrentTime() - KbootDriverState.cmdStartTime;
}

// ---------------------------------------------------------------------------
// Result helpers
// ---------------------------------------------------------------------------

static slave_result_t holdResult(void)
{
    return (slave_result_t){ .status = kStatus_Uhk_IdleCycle, .hold = true };
}

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

// Pre-format hex into a static buffer, then log in a single call.
// Max 38 bytes (largest framing packet) = 38*3 + label + overhead < 200 chars.
static char logFmtBuf[200];

static void logBuffer(const char *label, const uint8_t *buf, uint8_t len)
{
    int pos = 0;
    for (uint8_t i = 0; i < len && pos < (int)sizeof(logFmtBuf) - 4; i++) {
        pos += snprintf(logFmtBuf + pos, sizeof(logFmtBuf) - pos, " %02x", buf[i]);
    }
    LogU("Kboot: %s:%s\n", label, logFmtBuf);
}

static bool bufferIsNonZero(const uint8_t *buf, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        if (buf[i] != 0) {
            return true;
        }
    }
    return false;
}

static const char *cmdName(kboot_cmd_id_t id)
{
    switch (id) {
        case KbootCmdId_Erase: return "Erase";
        case KbootCmdId_WriteMemory: return "Write";
        case KbootCmdId_Reset: return "Reset";
        default: return "?";
    }
}

// ---------------------------------------------------------------------------
// Packet building
// ---------------------------------------------------------------------------

static uint8_t buildFramingPacket(uint8_t packetType, const uint8_t *payload, uint16_t len)
{
    txBuffer[0] = 0x5a;
    txBuffer[1] = packetType;
    txBuffer[2] = len & 0xff;
    txBuffer[3] = (len >> 8) & 0xff;

    crc16_data_t crc;
    crc16_init(&crc);
    crc16_update(&crc, txBuffer, 4);
    crc16_update(&crc, payload, len);
    uint16_t crcValue;
    crc16_finalize(&crc, &crcValue);

    txBuffer[4] = crcValue & 0xff;
    txBuffer[5] = (crcValue >> 8) & 0xff;
    memcpy(txBuffer + 6, payload, len);

    return 6 + len;
}

static uint32_t getResponseStatus(void)
{
    return rxBuffer[10] | ((uint32_t)rxBuffer[11] << 8) |
           ((uint32_t)rxBuffer[12] << 16) | ((uint32_t)rxBuffer[13] << 24);
}

// Verify CRC of a framing packet in rxBuffer. Returns true if valid.
static bool verifyRxCrc(uint8_t totalLen)
{
    uint16_t payloadLen = rxBuffer[2] | ((uint16_t)rxBuffer[3] << 8);
    uint16_t rxCrc = rxBuffer[4] | ((uint16_t)rxBuffer[5] << 8);
    crc16_data_t crc;
    crc16_init(&crc);
    crc16_update(&crc, rxBuffer, 4);
    if (6 + payloadLen <= totalLen) {
        crc16_update(&crc, rxBuffer + 6, payloadLen);
    }
    uint16_t computedCrc;
    crc16_finalize(&crc, &computedCrc);
    if (rxCrc != computedCrc) {
        LogU("Kboot: CRC mismatch: rx=0x%04x computed=0x%04x\n", rxCrc, computedCrc);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Abort / address helpers
// ---------------------------------------------------------------------------

static void togglePingAddress(void)
{
    KbootDriverState.i2cAddress = KbootDriverState.i2cAddress == I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER
        ? KBOOT_DEFAULT_I2C_ADDRESS
        : I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER;
}

typedef void (*abort_fn_t)(const char *);

static void abortFlash(const char *reason)
{
    LogU("Kboot: ABORT - %s\n", reason);
    ModuleFlashState = ModuleFlashState_Error;
    ModuleFlashBusy = false;
    KbootDriverState.command = KbootCommand_Idle;
    KbootDriverState.phase = 0;
}

static void abortReset(const char *reason)
{
    LogU("Kboot: ABORT - %s\n", reason);
    KbootDriverState.command = KbootCommand_Idle;
    KbootDriverState.phase = 0;
}

// ---------------------------------------------------------------------------
// Shared command transaction
//
// Every kboot command follows: TX cmd -> RX ACK (2B) -> RX response (18B) -> TX ACK (2B)
// Caller fills txBuffer, then calls startCmdTransaction().
// After completion, jumps to phaseAfterCmd.
// ---------------------------------------------------------------------------

static void startCmdTransaction(kboot_cmd_id_t cmdId, uint8_t phaseAfterCmd, uint32_t timeoutMs)
{
    KbootDriverState.cmdId = cmdId;
    KbootDriverState.phaseAfterCmd = phaseAfterCmd;
    KbootDriverState.cmdTimeoutMs = timeoutMs;
    KbootDriverState.cmdStartTime = Timer_GetCurrentTime();
    KbootDriverState.phase = KbootCmdPhase_Tx;
}

// Returns true if the phase was handled
static bool handleCmdTransaction(slave_result_t *res)
{
    if (KbootDriverState.phase < KbootCmdPhase_Tx || KbootDriverState.phase > KbootCmdPhase_TxAck) {
        return false;
    }

    // Use the appropriate abort for the active command
    abort_fn_t cmdAbort = KbootDriverState.command == KbootCommand_Flash ? abortFlash : abortReset;

    switch (KbootDriverState.phase) {
        case KbootCmdPhase_Tx: {
            uint16_t payloadLen = txBuffer[2] | ((uint16_t)txBuffer[3] << 8);
            uint8_t totalLen = 6 + payloadLen;
            logBuffer("TX cmd", txBuffer, totalLen);
            res->status = i2cTx(txBuffer, totalLen);
            KbootDriverState.phase = KbootCmdPhase_CheckTx;
            break;
        }

        case KbootCmdPhase_CheckTx: {
            status_t s = previousI2cStatus();
            LogU("Kboot: [%s] TX i2c=%d\n", cmdName(KbootDriverState.cmdId), s);
            if (s != kStatus_Success) {
                cmdAbort("Command TX failed");
                break;
            }
            KbootDriverState.phase = KbootCmdPhase_RxAck;
            *res = holdResult();
            break;
        }

        case KbootCmdPhase_RxAck:
            res->status = i2cRx(KBOOT_PACKAGE_LENGTH_ACK);
            KbootDriverState.phase = KbootCmdPhase_CheckAck;
            break;

        case KbootCmdPhase_CheckAck: {
            status_t s = previousI2cStatus();
            if (rxBuffer[0] == 0x5a && rxBuffer[1] == 0xa1) {
                LogU("Kboot: [%s] ACK OK (i2c=%d)\n", cmdName(KbootDriverState.cmdId), s);
                KbootDriverState.phase = KbootCmdPhase_RxResponse;
            } else {
                if (bufferIsNonZero(rxBuffer, KBOOT_PACKAGE_LENGTH_ACK)) {
                    logBuffer("ACK poll", rxBuffer, KBOOT_PACKAGE_LENGTH_ACK);
                }
                if (cmdElapsedMs() > KbootDriverState.cmdTimeoutMs) {
                    cmdAbort("ACK timeout");
                    break;
                }
                KbootDriverState.phase = KbootCmdPhase_RxAck;
            }
            *res = holdResult();
            break;
        }

        case KbootCmdPhase_RxResponse:
            res->status = i2cRx(KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
            KbootDriverState.phase = KbootCmdPhase_CheckResponse;
            break;

        case KbootCmdPhase_CheckResponse: {
            if (rxBuffer[0] == 0x5a && rxBuffer[1] == 0xa4) {
                if (!verifyRxCrc(KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE)) {
                    logBuffer("Response CRC bad", rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                    if (cmdElapsedMs() > KbootDriverState.cmdTimeoutMs) {
                        cmdAbort("Response timeout (CRC mismatches)");
                        break;
                    }
                    KbootDriverState.phase = KbootCmdPhase_RxResponse;
                    *res = holdResult();
                    break;
                }

                logBuffer("RX response", rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                uint32_t respStatus = getResponseStatus();
                LogU("Kboot: [%s] Response status=0x%x (%dms)\n",
                     cmdName(KbootDriverState.cmdId), respStatus, cmdElapsedMs());
                if (respStatus != 0) {
                    cmdAbort("Command failed");
                    break;
                }
                KbootDriverState.phase = KbootCmdPhase_TxAck;
            } else {
                if (bufferIsNonZero(rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE)) {
                    logBuffer("Response poll", rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                }
                if (cmdElapsedMs() > KbootDriverState.cmdTimeoutMs) {
                    logBuffer("Response(timeout)", rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                    cmdAbort("Response timeout");
                    break;
                }
                KbootDriverState.phase = KbootCmdPhase_RxResponse;
            }
            *res = holdResult();
            break;
        }

        case KbootCmdPhase_TxAck:
            LogU("Kboot: [%s] Done, sending ACK\n", cmdName(KbootDriverState.cmdId));
            res->status = i2cTx(ackMessage, sizeof(ackMessage));
            KbootDriverState.phase = KbootDriverState.phaseAfterCmd;
            break;
    }

    return true;
}

// ---------------------------------------------------------------------------
// Command builders (fill txBuffer, return total length)
// ---------------------------------------------------------------------------

static uint8_t buildEraseCommand(void)
{
    uint8_t payload[] = {0x0d, 0x00, 0x00, 0x00};
    return buildFramingPacket(0xa4, payload, sizeof(payload));
}

static uint8_t buildWriteMemoryCommand(uint32_t startAddress, uint32_t byteCount)
{
    uint8_t payload[12];
    payload[0] = 0x04;  // WriteMemory tag
    payload[1] = 0x01;  // HasDataPhase
    payload[2] = 0x00;
    payload[3] = 0x02;  // paramCount
    payload[4] = startAddress & 0xff;
    payload[5] = (startAddress >> 8) & 0xff;
    payload[6] = (startAddress >> 16) & 0xff;
    payload[7] = (startAddress >> 24) & 0xff;
    payload[8] = byteCount & 0xff;
    payload[9] = (byteCount >> 8) & 0xff;
    payload[10] = (byteCount >> 16) & 0xff;
    payload[11] = (byteCount >> 24) & 0xff;
    return buildFramingPacket(0xa4, payload, sizeof(payload));
}

static uint8_t buildResetCommand(void)
{
    uint8_t payload[] = {0x0b, 0x00, 0x00, 0x00};
    return buildFramingPacket(0xa4, payload, sizeof(payload));
}

// ---------------------------------------------------------------------------
// Ping handler (used by Flash and Reset)
//
// Returns true if ping phase was handled. On success, sets phase to nextPhase.
// On timeout, aborts via abortFn.
// ---------------------------------------------------------------------------

static bool handlePing(slave_result_t *res, uint8_t sendPingPhase, uint8_t nextPhase, abort_fn_t abortFn)
{
    uint8_t offset = KbootDriverState.phase - sendPingPhase;
    if (offset > 3) {
        return false;
    }

    switch (offset) {
        case 0: // SendPing
            pingAttemptCount++;
            if (shouldLogPing()) {
                LogU("Kboot: Ping #%u TX -> 0x%02x (%ums)\n",
                     pingAttemptCount, KbootDriverState.i2cAddress, elapsedMs());
            }
            res->status = i2cTx(pingCommand, sizeof(pingCommand));
            KbootDriverState.phase = sendPingPhase + 1;
            break;
        case 1: // CheckPingStatus
            KbootDriverState.status = previousI2cStatus();
            if (KbootDriverState.status == kStatus_Success) {
                KbootDriverState.phase = sendPingPhase + 2;
            } else if (elapsedMs() > KBOOT_PING_TIMEOUT_MS) {
                abortFn("Ping timeout");
                break;
            } else {
                if (shouldLogPing()) {
                    LogU("Kboot: Ping #%u TX failed status=0x%x, toggle addr\n",
                         pingAttemptCount, KbootDriverState.status);
                }
                togglePingAddress();
                KbootDriverState.phase = sendPingPhase;
            }
            *res = holdResult();
            break;
        case 2: // ReceivePingResponse
            res->status = i2cRx(KBOOT_PACKAGE_LENGTH_PING_RESPONSE);
            KbootDriverState.phase = sendPingPhase + 3;
            break;
        case 3: // CheckPingResponseStatus
            KbootDriverState.status = previousI2cStatus();
            if (KbootDriverState.status == kStatus_Success) {
                logBuffer("Ping response", rxBuffer, KBOOT_PACKAGE_LENGTH_PING_RESPONSE);
                LogU("Kboot: Ping OK at 0x%02x (%ums, %u attempts)\n",
                     KbootDriverState.i2cAddress, elapsedMs(), pingAttemptCount);
                KbootDriverState.phase = nextPhase;
            } else if (elapsedMs() > KBOOT_PING_TIMEOUT_MS) {
                abortFn("Ping response timeout");
                break;
            } else {
                if (shouldLogPing()) {
                    LogU("Kboot: Ping #%u RX failed status=0x%x, toggle addr\n",
                         pingAttemptCount, KbootDriverState.status);
                }
                togglePingAddress();
                KbootDriverState.phase = sendPingPhase;
            }
            *res = holdResult();
            break;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void KbootSlaveDriver_Init(uint8_t kbootInstanceId)
{
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

slave_result_t KbootSlaveDriver_Update(uint8_t kbootInstanceId)
{
    slave_result_t res = { .status = kStatus_Uhk_IdleSlave, .hold = false };

    // Shared command transaction (used by Flash and Reset)
    if (handleCmdTransaction(&res)) {
        return res;
    }

    switch (KbootDriverState.command) {
        case KbootCommand_Idle:
            break;

        case KbootCommand_Ping:
            switch (KbootDriverState.phase) {
                case KbootPhase_SendPing:
                    res.status = i2cTx(pingCommand, sizeof(pingCommand));
                    KbootDriverState.phase = KbootPhase_CheckPingStatus;
                    break;
                case KbootPhase_CheckPingStatus:
                    KbootDriverState.status = previousI2cStatus();
                    KbootDriverState.phase = KbootDriverState.status == kStatus_Success
                        ? KbootPhase_ReceivePingResponse
                        : KbootPhase_SendPing;
                    res = holdResult();
                    break;
                case KbootPhase_ReceivePingResponse:
                    res.status = i2cRx(KBOOT_PACKAGE_LENGTH_PING_RESPONSE);
                    KbootDriverState.phase = KbootPhase_CheckPingResponseStatus;
                    break;
                case KbootPhase_CheckPingResponseStatus:
                    KbootDriverState.status = previousI2cStatus();
                    if (KbootDriverState.status == kStatus_Success) {
                        KbootDriverState.command = KbootCommand_Idle;
                    } else {
                        KbootDriverState.phase = KbootPhase_SendPing;
                        res = holdResult();
                    }
                    break;
            }
            break;

        case KbootCommand_Reset:
            if (handlePing(&res, KbootResetPhase_SendPing, KbootResetPhase_SendReset, abortReset)) {
                if (KbootDriverState.phase == KbootResetPhase_SendReset) {
                    LogU("Kboot: Sending reset\n");
                    buildResetCommand();
                    startCmdTransaction(KbootCmdId_Reset, KbootResetPhase_Done, KBOOT_CMD_TIMEOUT_MS);
                }
                break;
            }

            switch (KbootDriverState.phase) {
                case KbootResetPhase_JumpToBootloader:
                    LogU("Kboot: Jumping to bootloader for Reset\n");
                    Slaves[SlaveId_RightModule].isConnected = true;
                    UhkModuleStates[UhkModuleDriverId_RightModule].phase = UhkModulePhase_JumpToBootloader;
                    KbootDriverState.i2cAddress = I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER;
                    KbootDriverState.startTime = Timer_GetCurrentTime();
                    KbootDriverState.phase = KbootResetPhase_WaitForBootloader;
                    pingAttemptCount = 0;
                    break;

                case KbootResetPhase_WaitForBootloader:
                    if (elapsedMs() < KBOOT_WAIT_AFTER_JUMP_MS) {
                        break;
                    }
                    LogU("Kboot: Wait done (%dms), pinging at 0x%02x/0x%02x\n",
                         KBOOT_WAIT_AFTER_JUMP_MS,
                         I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER, KBOOT_DEFAULT_I2C_ADDRESS);
                    KbootDriverState.phase = KbootResetPhase_SendPing;
                    break;

                case KbootResetPhase_Done:
                    LogU("Kboot: Reset complete (%ums)\n", elapsedMs());
                    KbootDriverState.command = KbootCommand_Idle;
                    break;
            }
            break;

        case KbootCommand_Flash:
            // Ping phases
            if (handlePing(&res, KbootFlashPhase_SendPing, KbootFlashPhase_StartWrite, abortFlash)) {
                // On ping success, start erase
                if (KbootDriverState.phase == KbootFlashPhase_StartWrite) {
                    KbootDriverState.startTime = Timer_GetCurrentTime();
                    KbootDriverState.firmwareOffset = 0;
                    ModuleFlashState = ModuleFlashState_Erasing;
                    buildEraseCommand();
                    LogU("Kboot: Starting erase\n");
                    startCmdTransaction(KbootCmdId_Erase, KbootFlashPhase_StartWrite, KBOOT_ERASE_TIMEOUT_MS);
                }
                break;
            }

            switch (KbootDriverState.phase) {

                // ===========================================================
                // Jump to bootloader
                // ===========================================================

                case KbootFlashPhase_JumpToBootloader: {
                    config_buffer_t *buf = ConfigBufferIdToConfigBuffer(ConfigBufferId_ModuleFirmware);
                    KbootDriverState.firmwareData = buf->buffer;
                    KbootDriverState.firmwareSize = ModuleFirmwareValidatedSize;
                    if (KbootDriverState.firmwareSize == 0) {
                        abortFlash("No validated firmware (size=0)");
                        break;
                    }
                    LogU("Kboot: Firmware ready, %u bytes. Jumping to bootloader\n",
                         KbootDriverState.firmwareSize);
                    // Force RightModule connected so the scheduler doesn't call
                    // UhkModuleSlaveDriver_Init (which would reset the phase we
                    // are about to set back to RequestSync).
                    Slaves[SlaveId_RightModule].isConnected = true;
                    UhkModuleStates[UhkModuleDriverId_RightModule].phase = UhkModulePhase_JumpToBootloader;
                    KbootDriverState.i2cAddress = I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER;
                    KbootDriverState.startTime = Timer_GetCurrentTime();
                    KbootDriverState.phase = KbootFlashPhase_WaitForBootloader;
                    pingAttemptCount = 0;
                    break;
                }

                case KbootFlashPhase_WaitForBootloader:
                    if (elapsedMs() < KBOOT_WAIT_AFTER_JUMP_MS) {
                        break;
                    }
                    LogU("Kboot: Wait done (%dms), pinging at 0x%02x/0x%02x\n",
                         KBOOT_WAIT_AFTER_JUMP_MS,
                         I2C_ADDRESS_RIGHT_MODULE_BOOTLOADER, KBOOT_DEFAULT_I2C_ADDRESS);
                    KbootDriverState.phase = KbootFlashPhase_SendPing;
                    break;

                // ===========================================================
                // Transition: erase done -> start WriteMemory command
                // ===========================================================

                case KbootFlashPhase_StartWrite: {
                    LogU("Kboot: Erase done. WriteMemory (addr=0x0, size=%u)\n",
                         KbootDriverState.firmwareSize);
                    ModuleFlashState = ModuleFlashState_Writing;
                    buildWriteMemoryCommand(0x0, KbootDriverState.firmwareSize);
                    startCmdTransaction(KbootCmdId_WriteMemory, KbootFlashPhase_SendDataChunk, KBOOT_CMD_TIMEOUT_MS);
                    break;
                }

                // ===========================================================
                // Data chunk loop
                // ===========================================================

                case KbootFlashPhase_SendDataChunk: {
                    uint32_t remaining = KbootDriverState.firmwareSize - KbootDriverState.firmwareOffset;
                    uint16_t chunkSize = remaining > KBOOT_DATA_CHUNK_SIZE ? KBOOT_DATA_CHUNK_SIZE : remaining;
                    // Log first 3 data chunks to verify firmware data
                    if (KbootDriverState.firmwareOffset < 3 * KBOOT_DATA_CHUNK_SIZE) {
                        logBuffer("FW data", KbootDriverState.firmwareData + KbootDriverState.firmwareOffset, chunkSize);
                    }
                    uint8_t len = buildFramingPacket(0xa5,
                        KbootDriverState.firmwareData + KbootDriverState.firmwareOffset, chunkSize);
                    res.status = i2cTx(txBuffer, len);
                    KbootDriverState.phase = KbootFlashPhase_DataChunkCheckTx;
                    break;
                }

                case KbootFlashPhase_DataChunkCheckTx: {
                    status_t s = previousI2cStatus();
                    if (s != kStatus_Success) {
                        LogU("Kboot: Data TX failed (i2c=%d, offset=%u)\n",
                             s, KbootDriverState.firmwareOffset);
                        abortFlash("Data chunk TX failed");
                        break;
                    }
                    KbootDriverState.phase = KbootFlashPhase_DataChunkRxAck;
                    res = holdResult();
                    break;
                }

                case KbootFlashPhase_DataChunkRxAck:
                    res.status = i2cRx(KBOOT_PACKAGE_LENGTH_ACK);
                    KbootDriverState.phase = KbootFlashPhase_DataChunkCheckAck;
                    break;

                case KbootFlashPhase_DataChunkCheckAck: {
                    if (bufferIsNonZero(rxBuffer, KBOOT_PACKAGE_LENGTH_ACK) &&
                        !(rxBuffer[0] == 0x5a && rxBuffer[1] == 0xa1)) {
                        logBuffer("Data ACK", rxBuffer, KBOOT_PACKAGE_LENGTH_ACK);
                    }
                    uint32_t remaining = KbootDriverState.firmwareSize - KbootDriverState.firmwareOffset;
                    uint16_t chunkSize = remaining > KBOOT_DATA_CHUNK_SIZE ? KBOOT_DATA_CHUNK_SIZE : remaining;
                    KbootDriverState.firmwareOffset += chunkSize;
                    if (KbootDriverState.firmwareOffset % KBOOT_PROGRESS_LOG_BYTES < KBOOT_DATA_CHUNK_SIZE) {
                        LogU("Kboot: Write %u/%u\n",
                             KbootDriverState.firmwareOffset, KbootDriverState.firmwareSize);
                    }
                    if (KbootDriverState.firmwareOffset >= KbootDriverState.firmwareSize) {
                        LogU("Kboot: All data sent, awaiting final response\n");
                        KbootDriverState.cmdStartTime = Timer_GetCurrentTime();
                        KbootDriverState.cmdTimeoutMs = KBOOT_CMD_TIMEOUT_MS;
                        KbootDriverState.cmdId = KbootCmdId_WriteMemory;
                        KbootDriverState.phase = KbootFlashPhase_FinalRxResponse;
                    } else {
                        KbootDriverState.phase = KbootFlashPhase_SendDataChunk;
                    }
                    res = holdResult();
                    break;
                }

                // ===========================================================
                // Final response after all data chunks
                // ===========================================================

                case KbootFlashPhase_FinalRxResponse:
                    res.status = i2cRx(KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                    KbootDriverState.phase = KbootFlashPhase_FinalCheckResponse;
                    break;

                case KbootFlashPhase_FinalCheckResponse: {
                    if (rxBuffer[0] == 0x5a && rxBuffer[1] == 0xa4 &&
                        verifyRxCrc(KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE)) {
                        logBuffer("RX final", rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                        uint32_t respStatus = getResponseStatus();
                        LogU("Kboot: [Write] Final status=0x%x\n", respStatus);
                        if (respStatus != 0) {
                            abortFlash("WriteMemory data failed");
                            break;
                        }
                        KbootDriverState.phase = KbootFlashPhase_FinalTxAck;
                    } else {
                        if (bufferIsNonZero(rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE)) {
                            logBuffer("Final poll", rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                        }
                        if (cmdElapsedMs() > KbootDriverState.cmdTimeoutMs) {
                            logBuffer("Final(timeout)", rxBuffer, KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE);
                            abortFlash("Final response timeout");
                            break;
                        }
                        KbootDriverState.phase = KbootFlashPhase_FinalRxResponse;
                    }
                    res = holdResult();
                    break;
                }

                case KbootFlashPhase_FinalTxAck:
                    LogU("Kboot: Write complete, sending final ACK\n");
                    res.status = i2cTx(ackMessage, sizeof(ackMessage));
                    KbootDriverState.phase = KbootFlashPhase_StartReset;
                    break;

                // ===========================================================
                // Transition: write done -> start Reset command
                // ===========================================================

                case KbootFlashPhase_StartReset:
                    LogU("Kboot: Resetting module\n");
                    buildResetCommand();
                    startCmdTransaction(KbootCmdId_Reset, KbootFlashPhase_FlashDone, KBOOT_CMD_TIMEOUT_MS);
                    break;

                // ===========================================================
                // Flash complete
                // ===========================================================

                case KbootFlashPhase_FlashDone:
                    LogU("Kboot: Flash complete (%dms total)\n", elapsedMs());
                    ModuleFlashState = ModuleFlashState_Done;
                    ModuleFlashBusy = false;
                    KbootDriverState.command = KbootCommand_Idle;
                    break;
            }
            break;
    }

    return res;
}
