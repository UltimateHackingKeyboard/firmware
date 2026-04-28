#ifndef __KBOOT_DRIVER_H__
#define __KBOOT_DRIVER_H__

// Includes:

#ifndef __ZEPHYR__
    #include "fsl_common.h"
#endif
#include "slave_scheduler.h"

// Macros:

    #define KBOOT_PACKAGE_MAX_LENGTH 32
    #define KBOOT_PACKAGE_LENGTH_PING_RESPONSE 10
    #define KBOOT_PACKAGE_LENGTH_ACK 2
    #define KBOOT_PACKAGE_LENGTH_GENERIC_RESPONSE 18

// Typedefs:

    typedef enum {
        KbootDriverId_Singleton,
    } kboot_driver_id_t;

    typedef enum {
        KbootCommand_Idle,
        KbootCommand_Ping,
        KbootCommand_Reset,
        KbootCommand_Flash,
    } kboot_command_t;

    typedef enum {
        KbootPhase_SendPing,
        KbootPhase_CheckPingStatus,
        KbootPhase_ReceivePingResponse,
        KbootPhase_CheckPingResponseStatus,
    } kboot_ping_phase_t;

    typedef enum {
        KbootResetPhase_JumpToBootloader,
        KbootResetPhase_WaitForBootloader,
        KbootResetPhase_SendPing,
        KbootResetPhase_CheckPingStatus,
        KbootResetPhase_ReceivePingResponse,
        KbootResetPhase_CheckPingResponseStatus,
        KbootResetPhase_SendReset,
        KbootResetPhase_Done,
    } kboot_reset_phase_t;

    // Shared command transaction phases (reused by Flash and Reset).
    // Values 240-249 so they don't collide with command-specific phases.
    typedef enum {
        KbootCmdPhase_Tx = 240,
        KbootCmdPhase_CheckTx,
        KbootCmdPhase_RxAck,
        KbootCmdPhase_CheckAck,
        KbootCmdPhase_RxResponse,
        KbootCmdPhase_CheckResponse,
        KbootCmdPhase_TxAck,
    } kboot_cmd_phase_t;

    typedef enum {
        // Boot + ping
        KbootFlashPhase_JumpToBootloader,
        KbootFlashPhase_WaitForBootloader,
        KbootFlashPhase_SendPing,
        KbootFlashPhase_CheckPingStatus,
        KbootFlashPhase_ReceivePingResponse,
        KbootFlashPhase_CheckPingResponseStatus,
        // Transitions
        KbootFlashPhase_StartWrite,
        KbootFlashPhase_StartReset,
        KbootFlashPhase_FlashDone,
        // Data phase (loops)
        KbootFlashPhase_SendDataChunk,
        KbootFlashPhase_DataChunkCheckTx,
        KbootFlashPhase_DataChunkRxAck,
        KbootFlashPhase_DataChunkCheckAck,
        // Final response after all data
        KbootFlashPhase_FinalRxResponse,
        KbootFlashPhase_FinalCheckResponse,
        KbootFlashPhase_FinalTxAck,
    } kboot_flash_phase_t;

    typedef enum {
        KbootCmdId_Erase,
        KbootCmdId_WriteMemory,
        KbootCmdId_Reset,
    } kboot_cmd_id_t;

    typedef struct {
        kboot_command_t command;
        uint8_t i2cAddress;
        uint8_t phase;
        uint32_t status;
        uint32_t startTime;
        // Flash state
        const uint8_t *firmwareData;
        uint32_t firmwareSize;
        uint32_t firmwareOffset;
        // Shared command transaction state
        kboot_cmd_id_t cmdId;
        uint8_t phaseAfterCmd;
        uint32_t cmdStartTime;
        uint32_t cmdTimeoutMs;
    } kboot_driver_state_t;

// Variables:

    extern kboot_driver_state_t KbootDriverState;

// Functions:

    void KbootSlaveDriver_Init(uint8_t kbootInstanceId);
    slave_result_t KbootSlaveDriver_Update(uint8_t kbootInstanceId);

#endif
