#ifndef __CONNECTIONS_H__
#define __CONNECTIONS_H__

// Includes:

    #include <zephyr/bluetooth/bluetooth.h>
    #include <stdint.h>
    #include "device.h"
    #include "host_connection.h"

// Macros:

// Typedefs:

    typedef enum {
        ConnectionType_Empty = HostConnectionType_Empty,
        ConnectionType_UsbHidRight = HostConnectionType_UsbHidRight,
        ConnectionType_UsbHidLeft = HostConnectionType_UsbHidLeft,
        ConnectionType_BtHid = HostConnectionType_BtHid,
        ConnectionType_NewBtHid = HostConnectionType_NewBtHid,
        ConnectionType_NusDongle = HostConnectionType_Dongle,
        ConnectionType_NusLeft = HostConnectionType_Count,
        ConnectionType_NusRight,
        ConnectionType_UartLeft,
        ConnectionType_UartRight,
        ConnectionType_Unknown,
    } connection_type_t;

    typedef enum {
        ConnectionId_Invalid,
        ConnectionId_HostConnectionFirst,
        ConnectionId_HostConnectionLast = ConnectionId_HostConnectionFirst + HOST_CONNECTION_COUNT_MAX - 1,
        ConnectionId_UsbHidLeft,
        ConnectionId_UsbHidRight, //alias to some host connection
        ConnectionId_UartLeft,
        ConnectionId_UartRight,
        ConnectionId_NusServerLeft,
        ConnectionId_NusServerRight,
        ConnectionId_NusClientRight,
        ConnectionId_BtHid, //alias to some host connection
        ConnectionId_Count,
    } connection_id_t;

    typedef enum {
        ConnectionTarget_None,
        ConnectionTarget_Left,
        ConnectionTarget_Right,
        ConnectionTarget_Host,
        ConnectionTarget_Count,
    } connection_target_t;

    typedef enum {
        ConnectionChannel_Uart,
        ConnectionChannel_Bt,
        ConnectionChannel_Usb,
    } connection_channel_t;

    typedef enum {
        ConnectionState_Disconnected,
        ConnectionState_Connected,
        ConnectionState_Ready,
        ConnectionState_Count,
    } connection_state_t;

    typedef struct {
        uint8_t peerId;
        connection_state_t state;
        bool isAlias;
    } ATTR_PACKED connection_t;

// Variables:

    extern connection_id_t ActiveHostConnectionId;
    extern connection_id_t SelectedHostConnectionId;
    extern connection_t Connections[ConnectionId_Count];

// Functions:

    void Connections_SetState(connection_id_t connectionId, connection_state_t state);
    void Connections_SetPeerId(connection_id_t connectionId, uint8_t peerId);
    connection_type_t Connections_Type(connection_id_t connectionId);
    connection_target_t Connections_Target(connection_id_t connectionId);
    connection_target_t Connections_DeviceToTarget(device_id_t deviceId);

    connection_id_t Connections_GetNewBtHidConnectionId();
    connection_id_t Connections_GetConnectionIdByBtAddr(const bt_addr_le_t *addr);

    bool Connections_IsHostConnection(connection_id_t connectionId);
    bool Connections_IsReady(connection_id_t connectionId);

    void Connections_HandleSwitchover(connection_id_t connectionId, bool forceSwitch);

#endif // __CONNECTIONS_H__