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

    typedef void (slave_handler_t)(uint8_t);
    typedef void (*slave_driver_initializer_t)();

    typedef struct {
        uint8_t moduleId;  // This is a unique, per-module ID.
        slave_handler_t *slaveHandler;
        bool isConnected;
    } uhk_slave_t;

// Functions:

    void InitSlaveScheduler();
    void SetLeds(uint8_t ledBrightness);

#endif
