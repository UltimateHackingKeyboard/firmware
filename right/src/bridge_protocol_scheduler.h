#ifndef __BRIDGE_PROTOCOL_SCHEDULER_H__
#define __BRIDGE_PROTOCOL_SCHEDULER_H__

// Includes:

    #include "fsl_common.h"

// Typedefs:

    typedef enum {
        BridgeSlaveType_LedDriver,
        BridgeSlaveType_UhkModule,
        BridgeSlaveType_Touchpad
    } bridge_slave_type_t;

    typedef struct {
        uint8_t i2cAddress;
        bridge_slave_type_t type;
        bool isConnected;
    } bridge_slave_t;

// Functions:

    void InitBridgeProtocolScheduler();
    void SetLeds(uint8_t ledBrightness);

#endif
