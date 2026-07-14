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
#include "event_scheduler.h"
#include "timer.h"
#include <zephyr/logging/log.h>
#include "usb_state.h"
#include "bt_manager.h"

LOG_MODULE_REGISTER(Conn, LOG_LEVEL_INF);

connection_t Connections[ConnectionId_Count] = {
    [ConnectionId_UsbHidRight] = { .isAlias = true },
    [ConnectionId_BtHid] = { .isAlias = true },
};

connection_id_t LastHostConnectionId = ConnectionId_Invalid;
connection_id_t CurrentHostConnectionId = ConnectionId_HostConnectionFirst;
connection_id_t LastSelectedHostConnectionId = ConnectionId_Invalid;

// The old SelectedHostConnectionId carried an implicit "the user explicitly
// asked for this host" intent. Folding Selected into CurrentHostConnectionId
// (which is now also set automatically by fallback/boot) would lose that, so we
// track it separately. It is transient: set when the user explicitly selects a
// host, and cleared once that host becomes Ready or Current changes for any
// non-explicit reason. Its only role is protecting an explicit pick from being
// stolen by the automatic fallback.
static bool currentHostConnectionIsExplicit = false;

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

    const char* currentLabel = connectionId == CurrentHostConnectionId ? ", Current" : "";
    const char* explicitLabel = (connectionId == CurrentHostConnectionId && currentHostConnectionIsExplicit) ? ", Explicit" : "";

    if (isHostConnection) {
        host_connection_t* hc = HostConnection(connectionId);
        LOG_INF("%s: %d(%.*s, %s)%s%s%s%s", name, connectionId - ConnectionId_HostConnectionFirst, hc->name.end - hc->name.start, hc->name.start, getStateString(Connections[connectionId].state), peerLabel, peerString, currentLabel, explicitLabel);
    } else {
        LOG_INF("%s: (%s)%s%s%s%s", name, getStateString(Connections[connectionId].state), peerLabel, peerString, currentLabel, explicitLabel);
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

    // stateNotApplied lets Connections_SetStateAsync apply the state up front
    // (so observers see it immediately) while deferring the side effects below;
    // the deferred Connections_UpdateStates() then re-enters here to run them.
    if ( Connections[connectionId].state != state || Connections[connectionId].stateNotApplied ) {
        Connections[connectionId].state = state;
        Connections[connectionId].stateNotApplied = false;
        reportConnectionState(connectionId);

        Connections_ResetWatermarks(connectionId);

        if (Connections_Target(connectionId) == ConnectionTarget_Host && DEVICE_IS_UHK80_RIGHT) {
            Connections_HandleSwitchover(connectionId, false);
            // Connections_HandleSwitchover calls DeviceState_Update for us
        } else {
            DeviceState_Update(Connections_Target(connectionId));
        }

        if (connectionId == CurrentHostConnectionId) {
            WIDGET_REFRESH(&TargetWidget);
        }
    }
}

void Connections_SetStateAsync(connection_id_t connectionId, connection_state_t state) {
    connectionId = resolveAliases(connectionId);

    // State changes can be triggered from various contexts (e.g. Bluetooth
    // callbacks). Apply the new state immediately so observers see the correct
    // value right away, but defer the heavy side effects (switchover, device
    // state updates) to the event loop via Connections_UpdateStates().
    if ( Connections[connectionId].state != state ) {
        Connections[connectionId].state = state;
        Connections[connectionId].stateNotApplied = true;
        EventScheduler_Schedule(Timer_GetCurrentTime(), EventSchedulerEvent_ConnectionsUpdateState, "Connections update state");
    }
}

void Connections_UpdateStates(void) {
    for (connection_id_t connectionId = 0; connectionId < ConnectionId_Count; connectionId++) {
        if (Connections[connectionId].stateNotApplied) {
            Connections_SetState(connectionId, Connections[connectionId].state);
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
    bool isOldCurrent = oldConnectionId == CurrentHostConnectionId;
    bool isNewCurrent = newConnectionId == CurrentHostConnectionId;

    // Save both connection states
    uint8_t oldPeerId = Connections[oldConnectionId].peerId;
    connection_state_t oldState = Connections[oldConnectionId].state;
    bool oldStateNotApplied = Connections[oldConnectionId].stateNotApplied;

    uint8_t newPeerId = Connections[newConnectionId].peerId;
    connection_state_t newState = Connections[newConnectionId].state;
    bool newStateNotApplied = Connections[newConnectionId].stateNotApplied;

    ASSERT(oldPeerId == peerId);
    ASSERT(newPeerId == PeerIdUnknown || newPeerId > peerId);

    // Exchange connection data
    Connections[newConnectionId].peerId = oldPeerId;
    Connections[newConnectionId].state = oldState;
    Connections[newConnectionId].stateNotApplied = oldStateNotApplied;

    Connections[oldConnectionId].peerId = newPeerId;
    Connections[oldConnectionId].state = newState;
    Connections[oldConnectionId].stateNotApplied = newStateNotApplied;

    // Update peer references for both connections
    if (oldPeerId != PeerIdUnknown) {
        Peers[oldPeerId].connectionId = newConnectionId;
    }
    if (newPeerId != PeerIdUnknown) {
        Peers[newPeerId].connectionId = oldConnectionId;
    }

    // Update current connection ID
    if (isOldCurrent) {
        CurrentHostConnectionId = newConnectionId;
    } else if (isNewCurrent) {
        CurrentHostConnectionId = oldConnectionId;
    }

    if (isOldCurrent || isNewCurrent) {
        WIDGET_REFRESH(&TargetWidget);
        // The advertising icon depends on the current connection's type.
        WIDGET_REFRESH(&StatusWidget);
    }
}

bool Connections_IsHostConnection(connection_id_t connectionId) {
    return ConnectionId_HostConnectionFirst <= connectionId && connectionId <= ConnectionId_HostConnectionLast;
}

bool Connections_IsReady(connection_id_t connectionId) {
    connectionId = resolveAliases(connectionId);
    return Connections[connectionId].state == ConnectionState_Ready;
}

bool Connections_IsCurrentHost(connection_id_t connectionId) {
    return resolveAliases(connectionId) == CurrentHostConnectionId;
}

// True when Current points at a real (configured) host that is not connected yet
// - i.e. we are actively trying to (re)connect to it. Unlike the old
// "SelectedHostConnectionId != Invalid" test, Current is always a valid id, so
// we must exclude empty/unknown slots (e.g. the boot default when unconfigured).
// The Current/switchover concept only exists on the right half; on other
// devices Current stays at its unused default, so report "not connecting".
bool Connections_IsSelectedConnecting(void) {
    if (!DEVICE_IS_UHK80_RIGHT) {
        return false;
    }
    if (Connections_GetState(CurrentHostConnectionId) >= ConnectionState_Connected) {
        return false;
    }
    switch (Connections_Type(CurrentHostConnectionId)) {
        case ConnectionType_BtHid:
        case ConnectionType_NusDongle:
        case ConnectionType_Empty:
            return true;
        default:
            return false;
    }
}

static void setDongleToStandby(connection_id_t connectionId) {
    if (Connections_IsHostConnection(connectionId) && HostConnection(connectionId)->type == HostConnectionType_Dongle) {
        bool standby = true;
        Messenger_Send2(DeviceId_Uhk_Dongle, MessageId_StateSync, StateSyncPropertyId_DongleStandby, (const uint8_t*)&standby, 1);
    }
}

static void updateLastConnection(connection_id_t lastConnId, connection_id_t newConnId) {
    if (
            LastHostConnectionId != lastConnId
            && lastConnId != ConnectionId_Invalid
            && lastConnId != newConnId
       ) {
        LastHostConnectionId = lastConnId;
    }
}

// Find the first ready switchover-marked host - the fallback target used when
// an automatic Current connection is lost.
static connection_id_t findReadySwitchoverHost(void) {
    for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
        if (Connections[i].state == ConnectionState_Ready && HostConnection(i)->switchover) {
            return i;
        }
    }
    return ConnectionId_Invalid;
}

static void switchOver(connection_id_t connectionId, bool explicitlySelected) {
    if (connectionId != CurrentHostConnectionId) {
        setDongleToStandby(CurrentHostConnectionId);
    }

    updateLastConnection(CurrentHostConnectionId, connectionId);

    CurrentHostConnectionId = connectionId;
    currentHostConnectionIsExplicit = explicitlySelected;

    // The target may not be connected yet (explicit selection of an offline
    // host), in which case it has no peer to stamp.
    if (Connections[connectionId].peerId != PeerIdUnknown) {
        Peers[Connections[connectionId].peerId].lastSwitchover = k_uptime_get_32();
    }

    Hid_UpdateKeyboardLedsState();
    WIDGET_REFRESH(&TargetWidget);
    // The advertising icon depends on the current connection's type.
    WIDGET_REFRESH(&StatusWidget);
    BtManager_StartScanningAndAdvertisingAsync(false, "switchover");
}

void Connections_HandleSwitchover(connection_id_t connectionId, bool forceSwitch) {
    connectionId = resolveAliases(connectionId);

    // Once the explicitly-requested host is up, drop the "actively pursuing"
    // intent so the normal automatic fallback rules apply again.
    if (currentHostConnectionIsExplicit && Connections_IsReady(CurrentHostConnectionId)) {
        currentHostConnectionIsExplicit = false;
    }

    if (forceSwitch && Connections_IsHostConnection(connectionId)) {
        // Explicit user selection: adopt as Current even if it is not (yet)
        // connected; BLE will keep trying to reach it.
        switchOver(connectionId, true);
        reportConnectionState(connectionId);
    } else if (!currentHostConnectionIsExplicit && Connections_GetState(CurrentHostConnectionId) < ConnectionState_Connected) {
        // An automatic Current connection is not connected: a ready
        // switchover-marked host may take over. An explicitly-selected Current
        // is protected and keeps being pursued.
        connection_id_t fallback = findReadySwitchoverHost();
        if (fallback != ConnectionId_Invalid && fallback != CurrentHostConnectionId) {
            switchOver(fallback, false);
            reportConnectionState(fallback);
        }
    }

    DeviceState_Update(Connections_Target(connectionId));
}

void Connections_ClearExplicitSelection(void) {
    if (currentHostConnectionIsExplicit) {
        currentHostConnectionIsExplicit = false;
        // Now that the explicit pursuit is dropped, let the automatic fallback
        // rules run (they will take over if Current is not connected).
        Connections_HandleSwitchover(CurrentHostConnectionId, false);
    }
}

bool Connections_IsConnectionAwake(connection_id_t connectionId) {
    switch (Connections_Type(CurrentHostConnectionId)) {
        case ConnectionType_NusDongle:
            return DongleHostAwake;
        case ConnectionType_UsbHidRight:
            return DEVICE_IS_UHK80_RIGHT && UsbState_Awake && UsbState_TransportUp;
        case ConnectionType_UsbHidLeft:
            return DEVICE_IS_UHK80_LEFT && UsbState_Awake && UsbState_TransportUp;
        case ConnectionType_Unknown:
        case ConnectionType_Empty:
            return false;
        default:
            return true;
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
    LogTo(DEVICE_ID, target, "Oob pairing in progress: %d\n", BtPair_OobPairingInProgress);
    LogTo(DEVICE_ID, target, "Directed advertising enabled: %d\n", Cfg.Bt_DirectedAdvertisingAllowed);
    LogTo(DEVICE_ID, target, "New connection: %d\n", Bt_NewPairedDevice);

    HostConnections_ListKnownBleConnections(target);
    BtConn_ListAllBonds(target);
    BtConn_ListCurrentConnections(target);
    LogTo(DEVICE_ID, target, "----------------------\n");
}

