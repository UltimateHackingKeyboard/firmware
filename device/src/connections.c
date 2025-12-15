#include "connections.h"
#include "bt_conn.h"
#include "device.h"
#include "host_connection.h"
#include "device_state.h"
#include "host_connection.h"
#include "keyboard/oled/widgets/widget_store.h"
#include "messenger.h"
#include "state_sync.h"
#include <zephyr/bluetooth/addr.h>
#include "connections.h"
#include "stubs.h"
#include "usb_compatibility.h"
#include "logger.h"
#include <stdio.h>
#include "config_manager.h"
#include "bt_pair.h"

connection_t Connections[ConnectionId_Count] = {
    [ConnectionId_UsbHidRight] = { .isAlias = true },
    [ConnectionId_BtHid] = { .isAlias = true },
};

connection_id_t LastActiveHostConnectionId = ConnectionId_Invalid;
connection_id_t ActiveHostConnectionId = ConnectionId_Invalid;
connection_id_t SelectedHostConnectionId = ConnectionId_Invalid;
connection_id_t LastSelectedHostConnectionId = ConnectionId_Invalid;

static connection_id_t resolveAliases(connection_id_t connectionId) {
    if (!Connections[connectionId].isAlias) {
        return connectionId;
    }

    if (connectionId == ConnectionId_UsbHidRight) {
        for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
            if (HostConnection(i)->type == HostConnectionType_UsbHidRight) {
                return i;
            }
        }
    }

    if (connectionId == ConnectionId_BtHid) {
        for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
            if (Peers[peerId].conn && Connections_Type(Peers[peerId].connectionId) == ConnectionType_BtHid) {
                return Peers[peerId].connectionId;
            }
        }
    }

    return connectionId;
}

static const char* getStateString(connection_state_t state) {
    switch (state) {
        case ConnectionState_Disconnected:
            return "Disconnected";
        case ConnectionState_Connected:
            return "Connected";
        case ConnectionState_Ready:
            return "Ready";
        default:
            return "Invalid";
    }
}

static const char* getStaticName(connection_id_t connectionId) {
#define CASE_FOR(NAME) case ConnectionId_##NAME: return #NAME;
    switch (connectionId) {
        case ConnectionId_HostConnectionFirst ... ConnectionId_HostConnectionLast:
            return "Host";
        CASE_FOR(UsbHidLeft);
        CASE_FOR(UsbHidRight);
        CASE_FOR(UartLeft);
        CASE_FOR(UartRight);
        CASE_FOR(NusServerLeft);
        CASE_FOR(NusServerRight);
        CASE_FOR(NusClientRight);
        CASE_FOR(BtHid);
        CASE_FOR(Invalid);
        CASE_FOR(Count);
        default:
            return "Invalid";
    }
}

const char* Connections_GetStaticName(connection_id_t connectionId) {
    return getStaticName(connectionId);
}

static void reportConnectionState(connection_id_t connectionId, const char* message) {
    connectionId = resolveAliases(connectionId);

    bool isHostConnection = false;
    const char* name = "Invalid";

    switch (connectionId) {
        case ConnectionId_HostConnectionFirst ... ConnectionId_HostConnectionLast:
            isHostConnection = true;
            name = "Host";
            break;
        default:
            name = Connections_GetStaticName(connectionId);
            break;
    }

    int8_t peerId = Connections[connectionId].peerId;
    const char* peerLabel = "";
    const char* peerString = "";
    if (peerId != PeerIdUnknown) {
        peerLabel = ", Peer ";
        peerString = Peers[peerId].name;
    }

    const char* activeLabel = connectionId == ActiveHostConnectionId ? ", Active" : "";
    const char* selectedLabel = connectionId == SelectedHostConnectionId ? ", Selected" : "";

    if (isHostConnection) {
        host_connection_t* hc = HostConnection(connectionId);
        LogU("%s: %s%d(%.*s, %s)%s%s%s%s\n", message, name, connectionId - ConnectionId_HostConnectionFirst, hc->name.end - hc->name.start, hc->name.start, getStateString(Connections[connectionId].state), peerLabel, peerString, activeLabel, selectedLabel);
    } else {
        LogU("%s: %s(%s)%s%s%s%s\n", message, name, getStateString(Connections[connectionId].state), peerLabel, peerString, activeLabel, selectedLabel);
    }
}

void Connections_ResetWatermarks(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);

    Connections[connectionId].watermarks.txIdx = 0;
    Connections[connectionId].watermarks.rxIdx = 255;
}

void Connections_ReportState(connection_id_t connectionId) {
    reportConnectionState(connectionId, "Conn state");
}

connection_state_t Connections_GetState(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);
    return Connections[connectionId].state;
}

void Connections_SetState(connection_id_t connectionId, connection_state_t state) {
    connectionId = resolveAliases(connectionId);

    if ( Connections[connectionId].state != state ) {
        Connections[connectionId].state = state;
        reportConnectionState(connectionId, "Conn state");

        Connections_ResetWatermarks(connectionId);

        if (Connections_Target(connectionId) == ConnectionTarget_Host && DEVICE_IS_UHK80_RIGHT) {
            Connections_HandleSwitchover(connectionId, false);
            // Connections_HandleSwitchover calls DeviceState_Update for us
        } else {
            DeviceState_Update(Connections_Target(connectionId));
        }
    }
}

void Connections_SetPeerId(connection_id_t connectionId, uint8_t peerId) {
    Connections[connectionId].peerId = peerId;
}

connection_type_t Connections_Type(connection_id_t connectionId) {
    switch (connectionId) {
        case ConnectionId_HostConnectionFirst ... ConnectionId_HostConnectionLast: {
            host_connection_type_t hostConnectionType = HostConnection(connectionId)->type;
            switch (hostConnectionType) {
                case HostConnectionType_NewBtHid:
                    return ConnectionType_BtHid;
                default:
                    return (connection_type_t)hostConnectionType;
            }
        }
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
        case ConnectionId_MyModule:
            return ConnectionType_UartModule;
        case ConnectionId_Count:
        case ConnectionId_Invalid:
            return ConnectionType_Unknown;
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
                case HostConnectionType_Count:
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
            return ConnectionTarget_Host;
        case ConnectionId_MyModule:
            return ConnectionTarget_Module;
        case ConnectionId_Count:
        case ConnectionId_Invalid:
            return ConnectionTarget_None;
    }
    printk("Unhandled connectionId %d\n", connectionId);
    return ConnectionTarget_None;
}

static connection_id_t getConnIdByPeer(const bt_addr_le_t *addr, bool searchHosts) {
    uint8_t stopAt = searchHosts ? PeerCount : PeerIdFirstHost;
    for (uint8_t peerId = 0; peerId < stopAt; peerId++) {
        bool addressMatches = BtAddrEq(addr, &Peers[peerId].addr);
        bool isValidPeer = peerId < PeerIdFirstHost || Peers[peerId].conn;
        if (addressMatches && isValidPeer) {
            return Peers[peerId].connectionId;
        }
    }

    return ConnectionId_Invalid;
}

static connection_id_t getConnIdByAddr(const bt_addr_le_t *addr) {
    for (uint8_t connectionId = ConnectionId_HostConnectionFirst; connectionId <= ConnectionId_HostConnectionLast; connectionId++) {
        host_connection_t *hostConnection = HostConnection(connectionId);
        switch (hostConnection->type) {
            case HostConnectionType_NewBtHid:
            case HostConnectionType_Dongle:
            case HostConnectionType_BtHid:
                if (BtAddrEq(addr, &hostConnection->bleAddress)) {
                    return connectionId;
                }
                break;
            case HostConnectionType_Empty:
            case HostConnectionType_UsbHidLeft:
            case HostConnectionType_UsbHidRight:
            case HostConnectionType_Count:
                break;
        }
    }

    return ConnectionId_Invalid;
}

connection_id_t Connections_GetConnectionIdByBtAddr(const bt_addr_le_t *addr) {
    connection_id_t res = ConnectionId_Invalid;

    res = getConnIdByPeer(addr, true);
    if (res != ConnectionId_Invalid) {
        return res;
    }

    res = getConnIdByAddr(addr);
    if (res != ConnectionId_Invalid) {
        return res;
    }

    return ConnectionId_Invalid;
}

connection_id_t Connections_GetConnectionIdByHostAddr(const bt_addr_le_t *addr) {
    connection_id_t res = ConnectionId_Invalid;

    res = getConnIdByPeer(addr, false);
    if (res != ConnectionId_Invalid) {
        return res;
    }

    res = getConnIdByAddr(addr);
    if (res != ConnectionId_Invalid) {
        return res;
    }

    return ConnectionId_Invalid;
}

connection_id_t Connections_GetNewBtHidConnectionId() {
    for (uint8_t connectionId = ConnectionId_HostConnectionFirst; connectionId <= ConnectionId_HostConnectionLast; connectionId++) {
        host_connection_t *hostConnection = HostConnection(connectionId);
        switch (hostConnection->type) {
            case HostConnectionType_NewBtHid:
                return connectionId;
                break;
            default:
                break;
        }
    }
    return ConnectionId_Invalid;
}

void Connections_MoveConnection(uint8_t peerId, connection_id_t oldConnectionId, connection_id_t newConnectionId) {
    bool isActiveConnection = oldConnectionId == ActiveHostConnectionId;
    bool isSelectedConnection = oldConnectionId == SelectedHostConnectionId;

    Connections[newConnectionId].peerId = Connections[oldConnectionId].peerId;
    Connections[newConnectionId].state = Connections[oldConnectionId].state;

    Peers[Connections[oldConnectionId].peerId].connectionId = newConnectionId;

    Connections[oldConnectionId].peerId = PeerIdUnknown;
    Connections[oldConnectionId].state = ConnectionState_Disconnected;

    ActiveHostConnectionId = isActiveConnection ? newConnectionId : ActiveHostConnectionId;
    SelectedHostConnectionId = isSelectedConnection ? newConnectionId : SelectedHostConnectionId;

    if (isActiveConnection) {
        WIDGET_REFRESH(&TargetWidget);
    }
}

bool Connections_IsHostConnection(connection_id_t connectionId) {
    return ConnectionId_HostConnectionFirst <= connectionId && connectionId <= ConnectionId_HostConnectionLast;
}

bool Connections_IsReady(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);
    return Connections[connectionId].state == ConnectionState_Ready;
}

bool Connections_IsActiveHostConnection(connection_id_t connectionId) {
    return resolveAliases(connectionId) == ActiveHostConnectionId;
}

static void setCurrentDongleToStandby() {
    if (Connections_IsHostConnection(ActiveHostConnectionId) && HostConnection(ActiveHostConnectionId)->type == HostConnectionType_Dongle) {
        bool standby = true;
        Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_StateSync, StateSyncPropertyId_DongleStandby, (const uint8_t*)&standby, 1);
    }
}

static void updateLastConnection(connection_id_t lastConnId, connection_id_t newConnId) {
    if (
            LastActiveHostConnectionId != lastConnId
            && lastConnId != ConnectionId_Invalid
            && lastConnId != newConnId
       ) {
        LastActiveHostConnectionId = lastConnId;
    }
}

static void switchOver(connection_id_t connectionId) {
    updateLastConnection(ActiveHostConnectionId, connectionId);

    ActiveHostConnectionId = connectionId;
    Peers[Connections[connectionId].peerId].lastSwitchover = k_uptime_get_32();

    if (connectionId == SelectedHostConnectionId) {
        HostConnection_SetSelectedConnection(ConnectionId_Invalid);
    }

    UsbCompatibility_UpdateKeyboardLedsState();
}

void Connections_HandleSwitchover(connection_id_t connectionId, bool forceSwitch) {
    connectionId = resolveAliases(connectionId);
    bool isReady = Connections_IsReady(connectionId);

    // Unset if disconnected
    if (connectionId == ActiveHostConnectionId && !isReady) {
        updateLastConnection(ActiveHostConnectionId, ConnectionId_Invalid);
        ActiveHostConnectionId = ConnectionId_Invalid;
    }

    // Decide whether to switch to the supplied connection
    if (isReady && Connections_IsHostConnection(connectionId)) {
        host_connection_t *hostConnection = HostConnection(connectionId);
        bool connectionIsSelected = SelectedHostConnectionId == connectionId;
        bool noHostIsConnected = ActiveHostConnectionId == ConnectionId_Invalid;
        if (hostConnection->switchover || noHostIsConnected || forceSwitch || connectionIsSelected) {
            setCurrentDongleToStandby();
            switchOver(connectionId);
            reportConnectionState(connectionId, "Conn state");
        }
    }

    // If current target is not usable
    if (ActiveHostConnectionId == ConnectionId_Invalid) {
        // Find the first ready host connection
        for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
            if (Connections[i].state == ConnectionState_Ready) {
                switchOver(i);
                reportConnectionState(i, "Conn state");
                break;
            }
        }
    }

    DeviceState_Update(Connections_Target(connectionId));
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

void Connections_PrintInfo(log_target_t target) {
    LogTo(DEVICE_ID, target, "Connection info:\n");
    LogTo(DEVICE_ID, target, "----------------------\n");
    LogTo(DEVICE_ID, target, "Compiled   peripheral count: %d\n", CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT);
    LogTo(DEVICE_ID, target, "Configured peripheral count: %d\n", Cfg.Bt_MaxPeripheralConnections);
    LogTo(DEVICE_ID, target, "Pairing mode: %d\n", BtPair_PairingMode);
    LogTo(DEVICE_ID, target, "Directed advertising enabled: %d\n", Cfg.Bt_DirectedAdvertisingAllowed);
    HostConnections_ListKnownBleConnections(target);
    BtConn_ListAllBonds(target);
    BtConn_ListCurrentConnections(target);
    LogTo(DEVICE_ID, target, "----------------------\n");
}

