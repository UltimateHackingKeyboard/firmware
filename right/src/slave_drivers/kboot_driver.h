#ifndef __KBOOT_DRIVER_H__
#define __KBOOT_DRIVER_H__

// Includes:

    #include "fsl_common.h"

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
    } kboot_command_t;

    typedef enum {
        KbootPhase_SendPing,
        KbootPhase_CheckPingStatus,
        KbootPhase_ReceivePingResponse,
        KbootPhase_CheckPingResponseStatus,
    } kboot_ping_phase_t;

    typedef enum {
        KbootPhase_SendReset,
        KbootPhase_ReceiveResetAck,
        KbootPhase_ReceiveResetGenericResponse,
        KbootPhase_CheckResetSendAck,
    } kboot_reset_phase_t;

    typedef struct {
        kboot_command_t command;
        uint8_t i2cAddress;
        uint8_t phase;
        uint32_t status;
    } kboot_driver_state_t;

// Variables:

    extern kboot_driver_state_t KbootDriverState;

// Functions:

    void KbootSlaveDriver_Init(uint8_t kbootInstanceId);
    status_t KbootSlaveDriver_Update(uint8_t kbootInstanceId);

#endif
