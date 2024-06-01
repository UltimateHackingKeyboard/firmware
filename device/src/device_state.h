#ifndef __DEVICE_STATE_H__
#define __DEVICE_STATE_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "device.h"

// Macros:

    #define DEVICE_STATE_FIRST_DEVICE DeviceId_Uhk80_Left
    #define DEVICE_STATE_LAST_DEVICE DeviceId_Uhk_Dongle

// Typedefs:

// Functions:

    bool DeviceState_IsConnected(device_id_t deviceId);
    void DeviceState_TriggerUpdate();

// Variables:

#endif // __DEVICE_STATE_H__
