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
#include "hid/transport.h"
#include "logger.h"
#include <stdio.h>
#include "config_manager.h"
#include "bt_pair.h"
#include "usb_commands/usb_command_get_new_pairings.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Conn, LOG_LEVEL_INF);

connection_t Connections[ConnectionId_Count] = {
    [ConnectionId_UsbHidRight] = { .isAlias = true },
    [ConnectionId_BtHid] = { .isAlias = true },
};

connection_id_t LastActiveHostConnectionId = ConnectionId_Invalid;
connection_id_t ActiveHostConnectionId = ConnectionId_Invalid;

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

static void reportConnectionState(connection_id_t connectionId) {
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

    if (isHostConnection) {
        host_connection_t* hc = HostConnection(connectionId);
        LOG_INF("%s: %d(%.*s, %s)%s%s%s", name, connectionId - ConnectionId_HostConnectionFirst, hc->name.end - hc->name.start, hc->name.start, getStateString(Connections[connectionId].state), peerLabel, peerString, activeLabel);
    } else {
        LOG_INF("%s: (%s)%s%s%s", name, getStateString(Connections[connectionId].state), peerLabel, peerString, activeLabel);
    }
}

void Connections_ResetWatermarks(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);

    Connections[connectionId].watermarks.txIdx = 0;
    Connections[connectionId].watermarks.rxIdx = 255;
}

void Connections_ReportState(connection_id_t connectionId) {
    reportConnectionState(connectionId);
}

connection_state_t Connections_GetState(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);
    return Connections[connectionId].state;
}

void Connections_SetState(connection_id_t connectionId, connection_state_t state) {
    connectionId = resolveAliases(connectionId);

    if ( Connections[connectionId].state != state ) {
        Connections[connectionId].state = state;
        reportConnectionState(connectionId);

        Connections_ResetWatermarks(connectionId);

        if (Connections_Target(connectionId) == ConnectionTarget_Host && DEVICE_IS_UHK80_RIGHT) {
            Connections_HandleConnectionStateChange(connectionId);
            // Connections_HandleConnectionStateChange calls DeviceState_Update for us
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
                    return ConnectionType_Empty;
                case HostConnectionType_UnregisteredBtHid:
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
        case ConnectionId_Count:
        case ConnectionId_Invalid:
            return ConnectionType_Unknown;
            break;
    }
    LOG_ERR("Unhandled connectionId %d", connectionId);
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
                case HostConnectionType_UnregisteredBtHid:
                    return ConnectionTarget_Host;
                case HostConnectionType_Dongle:
                    return ConnectionTarget_Host;
                case HostConnectionType_NewBtHid:
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
        case ConnectionId_Count:
        case ConnectionId_Invalid:
            return ConnectionTarget_None;
    }
    LOG_ERR("Unhandled connectionId %d", connectionId);
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
            case HostConnectionType_Dongle:
            case HostConnectionType_BtHid:
            case HostConnectionType_UnregisteredBtHid:
                if (BtAddrEq(addr, &hostConnection->bleAddress)) {
                    return connectionId;
                }
                break;
            case HostConnectionType_Empty:
            case HostConnectionType_UsbHidLeft:
            case HostConnectionType_UsbHidRight:
            case HostConnectionType_NewBtHid:
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
    bool isOldActive = oldConnectionId == ActiveHostConnectionId;
    bool isNewActive = newConnectionId == ActiveHostConnectionId;

    // Save both connection states
    uint8_t oldPeerId = Connections[oldConnectionId].peerId;
    connection_state_t oldState = Connections[oldConnectionId].state;

    uint8_t newPeerId = Connections[newConnectionId].peerId;
    connection_state_t newState = Connections[newConnectionId].state;

    ASSERT(oldPeerId == peerId);
    ASSERT(newPeerId == PeerIdUnknown || newPeerId > peerId);

    // Exchange connection data
    Connections[newConnectionId].peerId = oldPeerId;
    Connections[newConnectionId].state = oldState;

    Connections[oldConnectionId].peerId = newPeerId;
    Connections[oldConnectionId].state = newState;

    // Update peer references for both connections
    if (oldPeerId != PeerIdUnknown) {
        Peers[oldPeerId].connectionId = newConnectionId;
    }
    if (newPeerId != PeerIdUnknown) {
        Peers[newPeerId].connectionId = oldConnectionId;
    }

    // Update active connection ID
    if (isOldActive) {
        ActiveHostConnectionId = newConnectionId;
    } else if (isNewActive) {
        ActiveHostConnectionId = oldConnectionId;
    }

    if (isOldActive || isNewActive) {
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

bool Connections_ActiveHostIsReady(void) {
    return ActiveHostConnectionId != ConnectionId_Invalid
        && Connections[ActiveHostConnectionId].state == ConnectionState_Ready;
}

static void setDongleToStandby(connection_id_t connectionId) {
    if (Connections_IsHostConnection(connectionId) && HostConnection(connectionId)->type == HostConnectionType_Dongle) {
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

// Switch the keyboard's active host to `connectionId`. This is the only path that
// mutates ActiveHostConnectionId. It is invoked exclusively from explicit user
// actions (key/macro switch, pairing complete, boot default). Automatic switchover
// based on connection events was removed (issue #1471).
void Connections_SwitchToHost(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);
    if (!Connections_IsHostConnection(connectionId)) {
        return;
    }
    if (connectionId == ActiveHostConnectionId) {
        return;
    }

    setDongleToStandby(ActiveHostConnectionId);
    updateLastConnection(ActiveHostConnectionId, connectionId);

    ActiveHostConnectionId = connectionId;
    if (Connections[connectionId].peerId != PeerIdUnknown) {
        Peers[Connections[connectionId].peerId].lastSwitchover = k_uptime_get_32();
    }

    Hid_UpdateKeyboardLedsState();
    reportConnectionState(connectionId);
    DeviceState_Update(Connections_Target(connectionId));
}

// Called on connection state changes. Active is sticky (never reset on disconnect),
// so we just propagate the state change for reporting and HID gating.
void Connections_HandleConnectionStateChange(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);
    reportConnectionState(connectionId);
    if (connectionId == ActiveHostConnectionId) {
        // Refresh OLED so the "not connected" suffix on the Active host updates.
        WIDGET_REFRESH(&TargetWidget);
    }
    DeviceState_Update(Connections_Target(connectionId));
}

// Pick the first non-Empty host slot as Active. Called once after the config has
// been parsed. No-op if Active was already set (e.g. config reload at runtime).
void Connections_InitDefaultActive(void) {
    if (ActiveHostConnectionId != ConnectionId_Invalid) {
        return;
    }
    for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
        if (HostConnection(i)->type != HostConnectionType_Empty) {
            ActiveHostConnectionId = i;
            reportConnectionState(i);
            return;
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

void Connections_PrintInfo(log_target_t target) {
    LogTo(DEVICE_ID, target, "Connection info:\n");
    LogTo(DEVICE_ID, target, "----------------------\n");
    LogTo(DEVICE_ID, target, "Compiled   peripheral count: %d\n", CONFIG_BT_CTLR_SDC_PERIPHERAL_COUNT);
    LogTo(DEVICE_ID, target, "Configured peripheral count: %d\n", Cfg.Bt_MaxPeripheralConnections);
    LogTo(DEVICE_ID, target, "Pairing mode: %d\n", BtPair_PairingMode);
    LogTo(DEVICE_ID, target, "Directed advertising enabled: %d\n", Cfg.Bt_DirectedAdvertisingAllowed);
    LogTo(DEVICE_ID, target, "New connection: %d\n", Bt_NewPairedDevice);

    HostConnections_ListKnownBleConnections(target);
    BtConn_ListAllBonds(target);
    BtConn_ListCurrentConnections(target);
    LogTo(DEVICE_ID, target, "----------------------\n");
}

