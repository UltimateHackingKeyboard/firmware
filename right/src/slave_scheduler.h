#ifndef __SLAVE_SCHEDULER_H__
#define __SLAVE_SCHEDULER_H__

// Includes:

    #include "fsl_common.h"

// Typedefs:

    typedef enum {
        UhkSlaveType_LedDriver,
        UhkSlaveType_UhkModule,
        UhkSlaveType_Touchpad
    } uhk_slave_type_t;

    typedef enum {
        SlaveId_LeftKeyboardHalf,
        SlaveId_RightLedDriver,
        SlaveId_LeftLedDriver,
    } slave_id_t;

    typedef void (slave_initializer_t)(uint8_t);
    typedef void (slave_updater_t)(uint8_t);

    typedef struct {
        uint8_t perDriverId;  // Identifies the slave instance on a per-driver basis
        slave_initializer_t *initializer;
        slave_updater_t *updater;
        bool isConnected;
    } uhk_slave_t;

// Variables:

    extern uhk_slave_t slaves[];

// Functions:

    void InitSlaveScheduler();
    void SetLeds(uint8_t ledBrightness);

#endif
