#ifndef __SLAVE_SCHEDULER_H__
#define __SLAVE_SCHEDULER_H__

// Includes:

    #include <stdbool.h>
    #include <stdint.h>

#ifdef __ZEPHYR__
    typedef int32_t status_t;
    #define MAKE_STATUS(group, code) ((((group)*100) + (code)))
#else
    #include "fsl_common.h"
#endif

// Macros:

    #define SLAVE_COUNT 8
    #define IS_VALID_SLAVE_ID(slaveId) (0 <= slaveId && slaveId < SLAVE_COUNT)
    #define IS_STATUS_I2C_ERROR(status) (kStatus_I2C_Busy <= status && status <= kStatus_I2C_Timeout && status != kStatus_Fail)

// Typedefs:

    typedef enum { // Slaves[] is meant to be indexed with these values
        SlaveId_LeftKeyboardHalf,
        SlaveId_LeftModule,
        SlaveId_RightModule,
        SlaveId_RightTouchpad,
        SlaveId_RightLedDriver,
        SlaveId_LeftLedDriver,
        SlaveId_ModuleLeftLedDriver,
        SlaveId_KbootDriver,
    } slave_id_t;

    typedef struct {
        status_t status;
        bool hold;
    } slave_result_t;

    typedef void (slave_init_t)(uint8_t);
    typedef slave_result_t (slave_update_t)(uint8_t);
    typedef void (slave_disconnect_t)(uint8_t);
    typedef void (slave_connect_t)(uint8_t);

    typedef struct {
        uint8_t perDriverId;  // Identifies the slave instance on a per-driver basis
        slave_init_t *init;
        slave_update_t *update;
        slave_disconnect_t *disconnect;
        slave_connect_t *connect;
        bool isConnected;
        status_t previousStatus;
    } uhk_slave_t;

    typedef enum {
        kStatusGroup_Uhk = 200,
    } uhk_status_group_t;

    typedef enum {
        kStatus_Uhk_IdleSlave = MAKE_STATUS(kStatusGroup_Uhk, 0), // Another slave should be scheduled
        kStatus_Uhk_IdleCycle = MAKE_STATUS(kStatusGroup_Uhk, 1), // The same slave should be rescheduled
    } uhk_status_t;

// Variables:

    extern uhk_slave_t Slaves[SLAVE_COUNT];
    extern uint32_t I2cSlaveScheduler_Counter;

// Functions:

    void InitSlaveScheduler(void);
    void SlaveSchedulerCallback(status_t status);


    void SlaveScheduler_ScheduleSingleTransfer(uint8_t slaveId);
    void SlaveScheduler_FinalizeTransfer(uint8_t slaveId, status_t status);

#endif
