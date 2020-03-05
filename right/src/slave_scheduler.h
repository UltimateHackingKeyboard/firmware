#ifndef __SLAVE_SCHEDULER_H__
#define __SLAVE_SCHEDULER_H__

// Includes:

    #include "fsl_common.h"
    #include "config.h"

// Macros:

    #define SLAVE_COUNT (sizeof(Slaves) / sizeof(uhk_slave_t))
    #define MAX_SLAVE_COUNT 6
    #define IS_VALID_SLAVE_ID(slaveId) (0 <= slaveId && slaveId <= MAX_SLAVE_COUNT)
    #define IS_STATUS_I2C_ERROR(status) (kStatus_I2C_Busy <= status && status <= kStatus_I2C_Timeout)

// Typedefs:

    typedef enum { // Slaves[] is meant to be indexed with these values
        SlaveId_LeftKeyboardHalf,
        SlaveId_LeftModule,
        SlaveId_RightModule,
        SlaveId_RightTouchpad,
        SlaveId_RightLedDriver,
        SlaveId_LeftLedDriver,
        SlaveId_KbootDriver,
    } slave_id_t;

    typedef void (slave_init_t)(uint8_t);
    typedef status_t (slave_update_t)(uint8_t);
    typedef void (slave_disconnect_t)(uint8_t);

    typedef struct {
        uint8_t perDriverId;  // Identifies the slave instance on a per-driver basis
        slave_init_t *init;
        slave_update_t *update;
        slave_disconnect_t *disconnect;
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

    extern uhk_slave_t Slaves[];
    extern uint32_t I2cSlaveScheduler_Counter;

// Functions:

    void InitSlaveScheduler(void);

#endif
