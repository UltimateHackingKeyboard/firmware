#include "connections.h"
#include "device.h"
#include "host_connection.h"
#include "device_state.h"

connection_t Connections[ConnectionId_Count] = {};

connection_id_t TargetConnectionId = ConnectionId_Invalid;

void Connections_SetState(connection_id_t connectionId, connection_state_t state) {
    if ( Connections[connectionId].state != state ) {
        Connections[connectionId].state = state;
        Connections_HandleSwitchover(connectionId);
        DeviceState_Update(Connections_Target(connectionId));
    }
}

void Connections_SetPeerId(connection_id_t connectionId, uint8_t peerId) {
    Connections[connectionId].peerId = peerId;
}

connection_type_t Connections_Type(connection_id_t connectionId) {
    switch (connectionId) {
        case ConnectionId_HostConnectionFirst ... ConnectionId_HostConnectionLast:
            return (connection_type_t)HostConnection(connectionId)->type;
        case ConnectionId_UsbHidLeft:
            return ConnectionType_UsbHidLeft;
        case ConnectionId_UsbHidRight:
            return ConnectionType_UsbHidRight;
        case ConnectionId_UartLeft:
            return ConnectionType_UartLeft;
        case ConnectionId_UartRight:
            return ConnectionType_UartRight;
        case ConnectionId_NusServerLeft:
            return ConnectionType_NusLeft;
        case ConnectionId_NusServerRight:
            return ConnectionType_NusRight;
        case ConnectionId_NusClientRight:
            return ConnectionType_NusRight;
        case ConnectionId_BtHid:
            return ConnectionType_BtHid;
        case ConnectionId_NewBtHid:
            return ConnectionType_NewBtHid;
        case ConnectionId_Count:
            break;
    }
    printk("Unhandled connectionId %d\n", connectionId);
    return ConnectionType_Unknown;
}

connection_target_t Connections_Target(connection_id_t connectionId) {
    switch (connectionId) {
        case ConnectionId_HostConnectionFirst ... ConnectionId_HostConnectionLast:
            switch (HostConnection(connectionId)->type) {
                case HostConnectionType_UsbHidLeft:
                case HostConnectionType_UsbHidRight:
                    return ConnectionTarget_Host;
                case HostConnectionType_BtHid:
                case HostConnectionType_NewBtHid:
                    return ConnectionTarget_Host;
                case HostConnectionType_Dongle:
                    return ConnectionTarget_Host;
                case HostConnectionType_Empty:
                    return ConnectionTarget_None;
            }
        case ConnectionId_UartLeft:
        case ConnectionId_NusServerLeft:
            return ConnectionTarget_Left;

        case ConnectionId_UartRight:
        case ConnectionId_NusServerRight:
        case ConnectionId_NusClientRight:
            return ConnectionTarget_Right;
        case ConnectionId_UsbHidRight:
        case ConnectionId_UsbHidLeft:
            return ConnectionTarget_Host;
        case ConnectionId_BtHid:
        case ConnectionId_NewBtHid:
            return ConnectionTarget_Host;
        case ConnectionId_Count:
        case ConnectionId_Invalid:
            return ConnectionTarget_None;
    }
    printk("Unhandled connectionId %d\n", connectionId);
    return ConnectionTarget_None;
}

connection_id_t Connections_GetConnectionIdByBtAddr(const bt_addr_le_t *addr) {
    for (uint8_t connectionId = ConnectionId_HostConnectionFirst; connectionId <= ConnectionId_HostConnectionLast; connectionId++) {
        host_connection_t *hostConnection = &HostConnections(connectionId);
        switch (hostConnection->type) {
            case HostConnectionType_Dongle:
            case HostConnectionType_BtHid:
            case HostConnectionType_NewBtHid:
                if (bt_addr_le_cmp(addr, &hostConnection->bleAddress) == 0) {
                    return connectionId;
                }
                break;
            case HostConnectionType_Empty:
            case HostConnectionType_UsbHidLeft:
            case HostConnectionType_UsbHidRight:
                break;
        }
    }

    for (uint8_t peerId = 0; peerId < PeerCount; peerId++) {
        if (bt_addr_le_cmp(addr, &Peers[peerId].addr) == 0) {
            return Peers[peerId].connectionId;
        }
    }

    return ConnectionId_Invalid;
}

bool Connections_IsHostConnection(connection_id_t connectionId) {
    return ConnectionId_HostConnectionFirst <= connectionId && connectionId <= ConnectionId_HostConnectionLast;
}

bool Connections_IsReady(connection_id_t connectionId) {
    return Connections[connectionId].state == ConnectionState_Ready;
}

static connection_id_t resolveAliases(connection_id_t connectionId) {
    if (connectionId == ConnectionId_UsbHidRight) {
        for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
            if (HostConnection(i)->type == HostConnectionType_UsbHidRight) {
                return i;
            }
        }
    }

    return connectionId;
}

void Connections_HandleSwitchover(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);

    // Decide whether to switch to the supplied connection
    if (Connections_IsHostConnection(connectionId) || Connections[connectionId].state == ConnectionState_Ready) {
        host_connection_t *hostConnection = HostConnection(connectionId);
        if (hostConnection->switchover || TargetConnectionId == ConnectionId_Invalid) {
            TargetConnectionId = connectionId;
        }
    }

    // If current target is not usable
    if (TargetConnectionId == ConnectionId_Invalid || Connections[TargetConnectionId].state != ConnectionState_Ready) {
        // Find the first ready host connection
        for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
            if (Connections[i].state == ConnectionState_Ready) {
                TargetConnectionId = i;
                break;
            }
        }
    }
}

connection_target_t Connections_DeviceToTarget(device_id_t deviceId) {
    switch (deviceId) {
        case DeviceId_Uhk80_Left:
            return ConnectionTarget_Left;
        case DeviceId_Uhk80_Right:
            return ConnectionTarget_Right;
        case DeviceId_Uhk_Dongle:
            return ConnectionTarget_Host;
        default:
            return ConnectionTarget_None;
    }
}
