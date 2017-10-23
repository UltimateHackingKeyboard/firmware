#ifndef __KBOOT_DRIVER_H__
#define __KBOOT_DRIVER_H__

// Includes:

    #include "fsl_common.h"

// Macros:

    #define MAX_KBOOT_COMMAND_LENGTH 32

// Typedefs:

    typedef enum {
        KbootDriverId_Singleton,
    } kboot_driver_id_t;

    typedef enum {
        KbootCommand_Idle,
        KbootCommand_Ping,
        KbootCommand_Reset,
    } kboot_command_t;

    typedef struct {
        kboot_command_t commandType;
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
