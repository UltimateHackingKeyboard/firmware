#ifndef __DEVICE_STATE_H__
#define __DEVICE_STATE_H__

// Includes:

    #include <inttypes.h>
    #include <stdbool.h>
    #include "device.h"

// Macros:

    #define DEVICE_STATE_FIRST_DEVICE DeviceId_Uhk80_Left
    #define DEVICE_STATE_LAST_DEVICE DeviceId_Uhk_Dongle


/*
    typedef enum {
        ConnectionType_None,
        ConnectionType_Uart,
        ConnectionType_Bt,
        ConnectionType_Usb,
        ConnectionType_Count,
    } connection_type_t;

    typedef enum {
        ConnectionId_Left,
        ConnectionId_Right,
        ConnectionId_Dongle,
        ConnectionId_UsbHid,
        ConnectionId_BluetoothHid,
        ConnectionId_Count,
        ConnectionId_Invalid = ConnectionId_Count,
    } connection_id_t;
    */

// Typedefs:

// Variables:

    extern bool DongleStandby;

// Functions:

    bool DeviceState_IsTargetConnected(uint8_t target);
    bool DeviceState_IsDeviceConnected(device_id_t deviceId);
    void DeviceState_Update(uint8_t connectionTarget);

#endif // __DEVICE_STATE_H__
