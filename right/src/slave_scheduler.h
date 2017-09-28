#ifndef __SLAVE_SCHEDULER_H__
#define __SLAVE_SCHEDULER_H__

// Includes:

    #include "fsl_common.h"

// Typedefs:

    typedef enum {
        SlaveId_LeftKeyboardHalf,
        SlaveId_RightLedDriver,
        SlaveId_LeftLedDriver,
    } slave_id_t;

    typedef void (slave_init_t)(uint8_t);
    typedef status_t (slave_update_t)(uint8_t);

    typedef struct {
        uint8_t perDriverId;  // Identifies the slave instance on a per-driver basis
        slave_init_t *init;
        slave_update_t *update;
        bool isConnected;
    } uhk_slave_t;

    typedef enum {
        kStatusGroup_Uhk = -1,
    } uhk_status_group_t;

    typedef enum {
        kStatus_Uhk_IdleSlave = MAKE_STATUS(kStatusGroup_Uhk, 0), // An other slave should be scheduled
        kStatus_Uhk_NoOp = MAKE_STATUS(kStatusGroup_Uhk, 1),      // The same slave should be rescheduled
    } uhk_status_t;

// Variables:

    extern uhk_slave_t Slaves[];
    extern uint32_t BridgeCounter;

// Functions:

    void InitSlaveScheduler();
    void SetLeds(uint8_t ledBrightness);

#endif
