#include "attributes.h"
#include "keyboard/oled/framebuffer.h"
#include "zephyr/bluetooth/hci_types.h"
#include <stdio.h>
#include <sys/types.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/conn.h>
#ifdef CONFIG_BT_SCAN
#include <bluetooth/scan.h>
#endif
#include "bt_advertise.h"
#include "bt_conn.h"
#include "bt_scan.h"
#include "connections.h"
#include "device_state.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/widgets/widget.h"
#include "event_scheduler.h"
#include "host_connection.h"
#include "nus_client.h"
#include "nus_server.h"
#include "device.h"
#include "keyboard/oled/screens/pairing_screen.h"
#include "usb/usb.h"
#include "keyboard/oled/widgets/widgets.h"
#include <zephyr/settings/settings.h>
#include "bt_pair.h"
#include "bt_manager.h"
#include <zephyr/bluetooth/addr.h>
#include "config_manager.h"
#include "zephyr/kernel.h"
#include <zephyr/bluetooth/gatt.h>
#include "stubs.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(Bt, LOG_LEVEL_WRN);

bool Bt_NewPairedDevice = false;

struct bt_conn *auth_conn;

#define BLE_KEY_LEN 16
#define BLE_ADDR_LEN 6

static void disconnectAllHids();

peer_t Peers[PeerCount] = {
    {
        .id = PeerIdUnknown,
        .name = "unknown",
        .connectionId = ConnectionId_Invalid,
    },
    {
        .id = PeerIdLeft,
        .name = "left",
        .connectionId = ConnectionId_NusServerLeft,
    },
    {
        .id = PeerIdRight,
        .name = "right",
#if DEVICE_IS_UHK_DONGLE
        .connectionId = ConnectionId_NusServerRight,
#elif DEVICE_IS_UHK80_LEFT
        .connectionId = ConnectionId_NusClientRight,
#endif
    },
};

peer_t *getPeerByAddr(const bt_addr_le_t *addr) {
    for (uint8_t i = PeerIdFirst; i < PeerCount; i++) {
        if (BtAddrEq(addr, &Peers[i].addr)) {
            return &Peers[i];
        }
    }

    return NULL;
}

peer_t *getPeerByConn(const struct bt_conn *conn) {
    for (uint8_t i = PeerIdFirst; i < PeerCount; i++) {
        if (conn == Peers[i].conn) {
            return &Peers[i];
        }
    }

    return NULL;
}

int8_t GetPeerIdByConn(const struct bt_conn *conn) {
    peer_t *peer = getPeerByConn(conn);
    int8_t peerId = peer ? peer->id : PeerIdUnknown;
    return peerId;
}

char* GetAddrString(const bt_addr_le_t *addr)
{
    static char addr_str[BT_ADDR_LE_STR_LEN]; // Length defined by Zephyr
    bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
    return addr_str;
}

char *GetPeerStringByAddr(const bt_addr_le_t *addr) {
    // determine address string
    char addrStr[BT_ADDR_STR_LEN];
    for (uint8_t i=0; i<BT_ADDR_SIZE; i++) {
        sprintf(&addrStr[i*3], "%02x:", addr->a.val[BT_ADDR_SIZE-1-i]);
    }
    addrStr[BT_ADDR_STR_LEN-1] = '\0';

    // determine peer name
    peer_t *peer = getPeerByAddr(addr);
    char peerName[PeerNameMaxLength];

    if (peer) {
        strcpy(peerName, peer->name);
    } else {
        strcpy(peerName, "n/a");
    }


    // determine user's host name
    #define MAX_HOST_NAME_LENGTH 16
    const char* unknown = "n/a";
    string_segment_t hostName = { .start = unknown, .end = unknown + strlen(unknown) };

    connection_id_t connId = ConnectionId_Invalid;
    if (peer) {
        connId = peer->connectionId;
    } else {
        connId = Connections_GetConnectionIdByHostAddr(addr);
    }

    host_connection_t *hostConnection = HostConnection(connId);
    if (hostConnection) {
        hostName = hostConnection->name;
    }

    // put it together
    static char peerString[PeerNameMaxLength + BT_ADDR_LE_STR_LEN + 6 + MAX_HOST_NAME_LENGTH];
    sprintf(peerString, "%s (%.*s, %s)", peerName, MIN(MAX_HOST_NAME_LENGTH, hostName.end - hostName.start), hostName.start, addrStr);

    return peerString;
}

char *GetPeerStringByConn(const struct bt_conn *conn) {
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    return GetPeerStringByAddr(addr);
}

static struct bt_conn_le_data_len_param *data_len;

static void enableDataLengthExtension(struct bt_conn *conn) {
    data_len = BT_LE_DATA_LEN_PARAM_MAX;

    int err = bt_conn_le_data_len_update(conn, data_len);
    if (err) {
        LOG_INF("LE data length update failed: %d", err);
    }
}

static void setLatency(struct bt_conn* conn, const struct bt_le_conn_param* params) {
    int err = bt_conn_le_param_update(conn, params);
    if (err) {
        LOG_WRN("LE latencies update failed: %d\n", err);
    }
}

static void configureLatency(struct bt_conn *conn, latency_mode_t latencyMode) {
    switch (latencyMode) {
        case LatencyMode_NUS: {
                // https://developer.apple.com/library/archive/qa/qa1931/_index.html
                // https://punchthrough.com/manage-ble-connection/
                // https://devzone.nordicsemi.com/f/nordic-q-a/28058/what-is-connection-parameters
                const struct bt_le_conn_param conn_params = BT_LE_CONN_PARAM_INIT(
                    6, 6, // keep it low, lowest allowed is 6 (7.5ms), lowest supported widely is 9 (11.25ms)
                    0, // keeping it higher allows power saving on peripheral when there's nothing to send (keep it under 30 though)
                    100 // connection timeout (*10ms)
                );
                setLatency(conn, &conn_params);
             }
            break;
        case LatencyMode_BleHid: {
                // https://developer.apple.com/library/archive/qa/qa1931/_index.html
                // https://punchthrough.com/manage-ble-connection/
                // https://devzone.nordicsemi.com/f/nordic-q-a/28058/what-is-connection-parameters
                const struct bt_le_conn_param conn_params = BT_LE_CONN_PARAM_INIT(
                    6, 9, // keep it low, lowest allowed is 6 (7.5ms), lowest supported widely is 9 (11.25ms)
                    0, // keeping it higher allows power saving on peripheral when there's nothing to send (keep it under 30 though)
                    100 // connection timeout (*10ms)
                );
                setLatency(conn, &conn_params);
             }
            break;
    }
}

static void youAreNotWanted(struct bt_conn *conn) {
// BT_HCI_ERR_REMOTE_USER_TERM_CONN
    static uint32_t lastAttemptTime = 0;
    uint32_t currentTime = k_uptime_get_32();

    if (currentTime - lastAttemptTime < 2000) {
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    } else {
        LOG_WRN("Refusing connenction %s (this is not a selected connection)\n", GetPeerStringByConn(conn));
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }

    lastAttemptTime = currentTime;
}

ATTR_UNUSED static void youAreNotAuthenticated(struct bt_conn *conn) {
    LOG_WRN("Implement this!\n");
}

void BtConn_UpdateHostConnectionPeerAllocations() {
    //for each peer
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        struct bt_conn* conn = Peers[peerId].conn;
        if (conn) {
            connection_id_t currentId = Peers[peerId].connectionId;
            connection_id_t newId = Connections_GetConnectionIdByHostAddr(bt_conn_get_dst(conn));
            LOG_INF("Reallocating peer %s from connection %d -> %d\n", Peers[peerId].name, currentId, newId);
            if (newId != ConnectionId_Invalid && newId != currentId) {
                Connections_MoveConnection(peerId, currentId, newId);
            }
        }
    }
}

static uint8_t allocateHostPeer(uint8_t connectionType) {
    switch (connectionType) {
        case ConnectionType_NusLeft:
            return PeerIdLeft;
        case ConnectionType_NusRight:
            return PeerIdRight;
        default:
            for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
                if (!Peers[peerId].conn) {
                    return peerId;
                }
            }
            break;
    }
    return PeerIdUnknown;
}

static uint8_t assignPeer(struct bt_conn* conn, uint8_t connectionId, uint8_t connectionType) {
    bt_addr_le_t addr = *bt_conn_get_dst(conn);

    // be idempotent
    for (uint8_t i = PeerIdFirst; i < PeerCount; i++) {
        if (Peers[i].conn == conn) {
            Peers[i].addr = addr;
            if (Peers[i].connectionId != connectionId) {
                Connections_MoveConnection(i, Peers[i].connectionId, connectionId);
            }
            return i;
        }
    }

    uint8_t peerId = allocateHostPeer(connectionType);

    if (peerId == PeerIdUnknown) {
        LOG_WRN("No peer slot available for connection %d\n", connectionId);
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return PeerIdUnknown;
    }

    Peers[peerId].conn = bt_conn_ref(conn);
    Peers[peerId].addr = addr;
    Peers[peerId].connectionId = connectionId;
    Connections[connectionId].peerId = peerId;
    Connections_SetState(connectionId, ConnectionState_Connected);

    return peerId;
}

void bt_foreach_list_current_connections(struct bt_conn *conn, void *data)
{
    int8_t peerId = GetPeerIdByConn(conn);
    if (peerId == PeerIdUnknown) {
        LogU("  - %s\n", GetPeerStringByConn(conn));
    } else {
        LogU("  - peer %d(%s), connection %d\n", peerId, GetPeerStringByConn(conn), Peers[peerId].connectionId);
    }
}

void BtConn_ListCurrentConnections() {
    LogU("Current connections:\n");
    bt_conn_foreach(BT_CONN_TYPE_LE, bt_foreach_list_current_connections, NULL);
}


static void bt_foreach_print_bond(const struct bt_bond_info *info, void *user_data)
{
    LogU(" - %s\n", GetAddrString(&info->addr));
}

void BtConn_ListAllBonds() {
    LogU("All bonds:\n");
    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_print_bond, NULL);
}

// If last available slot is reserved for a selected connection, refuse other connections
static bool isWanted(struct bt_conn *conn, connection_id_t connectionId, connection_type_t connectionType) {
    bool selectedConnectionIsBleHid = Connections_Type(SelectedHostConnectionId) == ConnectionType_BtHid;

    bool isHidCollision = connectionType == ConnectionType_BtHid && BtConn_ConnectedHidCount() > 0;
    bool isSelectedConnection = BtAddrEq(bt_conn_get_dst(conn), &HostConnection(SelectedHostConnectionId)->bleAddress);
    bool weHaveSlotToSpare = BtConn_UnusedPeripheralConnectionCount() > 1 || SelectedHostConnectionId == ConnectionId_Invalid;
    bool isLeftConnection = connectionType == ConnectionType_NusLeft;


    if (isHidCollision) {
    /**
     * TODO: uncomment and test this code if we want to allow inplace hid switchover initiated by the remote.
     *       I predict this is not workable due to aggressive connection policies of third parties.
        if (SelectedHostConnectionId == ConnectionId_Invalid) {
            host_connection_t *hostConnection = HostConnection(connectionId);
            bool wantSwitch = hostConnection != NULL && hostConnection->switchover;
            if (wantSwitch) {
                disconnectAllHids();
                return true;
            } else {
                return false;
            }
        } else {
            if (isSelectedConnection) {
                disconnectAllHids();
                return true;
            } else {
                return false;
            }
        }
    */
        return false;
    } else if (selectedConnectionIsBleHid) {
        return isSelectedConnection;
    }
    else {
        return weHaveSlotToSpare || isSelectedConnection || isLeftConnection;
    }
}

static void connectNus(struct bt_conn *conn, connection_id_t connectionId, connection_type_t connectionType) {
    uint8_t peerId = assignPeer(conn, connectionId, connectionType);

    LOG_INF("connected to %s\n", GetPeerStringByConn(conn));

    configureLatency(conn, LatencyMode_NUS);
    enableDataLengthExtension(conn);

    bool isRightClient = DEVICE_IS_UHK80_RIGHT && peerId == PeerIdLeft;
    bool isDongleClient = DEVICE_IS_UHK_DONGLE && peerId == PeerIdRight;
    if ( isRightClient || isDongleClient ) {
        LOG_INF("Initiating NUS connection with %s\n", GetPeerStringByConn(conn));
        NusClient_Connect(conn);
    }
}

static void connectHid(struct bt_conn *conn, connection_id_t connectionId, connection_type_t connectionType) {
    assignPeer(conn, connectionId, connectionType);

    configureLatency(conn, LatencyMode_NUS);

    // Assume that HOGP is ready
    LOG_INF("Established HID connection with %s\n", GetPeerStringByConn(conn));
    Connections_SetState(connectionId, ConnectionState_Ready);
}

#define BT_UUID_NUS_VAL BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_NUS BT_UUID_DECLARE_128(BT_UUID_NUS_VAL)

ATTR_UNUSED static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params)
{
    if (!attr) {
        LOG_INF("Service discovery completed, connection wasn't matched.\n");
        // TODO: consider setting a timer to disconnect the connection if neither auth nor security is Established
        return BT_GATT_ITER_STOP;
    }

    if (attr->user_data) {
        struct bt_gatt_service_val *service_val = (struct bt_gatt_service_val *)attr->user_data;
        if (service_val && service_val->uuid) {
            if (service_val->uuid->type == BT_UUID_TYPE_128 && !bt_uuid_cmp(service_val->uuid, BT_UUID_NUS)) {
                if (!BtPair_OobPairingInProgress && DEVICE_IS_UHK80_RIGHT) {
                    LOG_INF("Unknown NUS trying to connect. Refusing!\n");
                    bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
                }
                return BT_GATT_ITER_STOP;
            }
        }
    }

    return BT_GATT_ITER_CONTINUE;
}

// check if the connection is NUS. If yes, disconnect it.
// Otherwise, assume that this is anonymous ble hid connection, and
// it to (try to) increase its security.
static void connectUnknown(struct bt_conn *conn) {
#if DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE
    int err;

    LOG_INF("Bt connected to unknown. Starting discovery.\n");
    static struct bt_gatt_discover_params discover_params;
    discover_params.uuid = NULL;  // Will discover all services
    discover_params.start_handle = 0x0001;
    discover_params.end_handle = 0xFFFF;
    discover_params.func = discover_func;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(conn, &discover_params);
    if (err) {
        LOG_WRN("Service discovery failed (err %u)\n", err);
        return;
    }
#endif
}

static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        LOG_WRN("Failed to connect to %s, err %u\n", GetPeerStringByConn(conn), err);
        BtManager_StartScanningAndAdvertising();
        return;
    }

    const bt_addr_le_t * addr = bt_conn_get_dst(conn);
    connection_id_t connectionId = Connections_GetConnectionIdByHostAddr(addr);
    connection_type_t connectionType = Connections_Type(connectionId);

    LOG_INF("connected %s, %d %d\n", GetPeerStringByConn(conn), connectionId, connectionType);

    if (connectionId == ConnectionId_Invalid) {
        connectUnknown(conn);
        BtManager_StartScanningAndAdvertisingAsync();
    } else {

        if (isWanted(conn, connectionId, connectionType)) {
            bt_conn_set_security(conn, BT_SECURITY_L4);
            // advertising/scanning needs to be started only after peers are assigned :-/
        } else {
            youAreNotWanted(conn);
            BtManager_StartScanningAndAdvertisingAsync();
        }
    }



    return;
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    int8_t peerId = GetPeerIdByConn(conn);
    connection_type_t connectionType = Connections_Type(Peers[peerId].connectionId);

    ARG_UNUSED(peerId);

    LOG_INF("Bt disconnected from %s, reason %u\n", GetPeerStringByConn(conn), reason);

    if (DEVICE_IS_UHK80_LEFT && peerId == PeerIdRight) {
        NusServer_Disconnected();
    }
    if (DEVICE_IS_UHK80_RIGHT && connectionType == ConnectionType_NusDongle) {
        NusServer_Disconnected();
    }
    if (DEVICE_IS_UHK80_RIGHT && peerId == PeerIdLeft) {
        NusClient_Disconnected();
    }
    if (DEVICE_IS_UHK_DONGLE && peerId == PeerIdRight) {
        NusClient_Disconnected();
    }

    if (peerId != PeerIdUnknown) {
        Connections_SetState(Peers[peerId].connectionId, ConnectionState_Disconnected);
        bt_conn_unref(Peers[peerId].conn);
        Peers[peerId].conn = NULL;
        Connections[Peers[peerId].connectionId].peerId = PeerIdUnknown;
        if (peerId >= PeerIdFirstHost) {
            Peers[peerId].connectionId = ConnectionId_Invalid;
            memset(&Peers[peerId].addr, 0, sizeof(bt_addr_le_t));
        }
    }

    if (!BtPair_OobPairingInProgress && !BtManager_Restarting) {
        BtManager_StartScanningAndAdvertisingAsync();
    }

    if (conn == auth_conn) {
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }
}

void Bt_SetConnectionConfigured(struct bt_conn* conn) {
    uint8_t peerId = GetPeerIdByConn(conn);
    Connections_SetState(Peers[peerId].connectionId, ConnectionState_Ready);
}

static bool isUhkDeviceConnection(connection_type_t connectionType) {
    switch (connectionType) {
        case ConnectionType_NusLeft:
        case ConnectionType_NusRight:
        case ConnectionType_NusDongle:
            return true;
        default:
            return false;
    }
}

static void connectAuthenticatedConnection(struct bt_conn *conn, connection_id_t connectionId, connection_type_t connectionType) {
    // in case we don't have free connection slots and this is not the selected connection, then refuse
    if (!isWanted(conn, connectionId, connectionType)) {
        LOG_WRN("Refusing authenticated connenction %d (this is not a selected connection)\n", connectionId);
        youAreNotWanted(conn);
        return;
    }

    switch (connectionType) {
        case ConnectionType_NusLeft:
        case ConnectionType_NusRight:
        case ConnectionType_NusDongle:
            connectNus(conn, connectionId, connectionType);
            break;
        case ConnectionType_BtHid:
            connectHid(conn, connectionId, connectionType);
            break;
        case ConnectionType_Unknown:
        default:
            LOG_WRN("Authenticated connection is not known. Disconnecting %s", GetPeerStringByConn(conn));
            bt_conn_disconnect(conn, BT_HCI_ERR_AUTH_FAIL);
            break;
    }

    BtManager_StartScanningAndAdvertisingAsync();
}

static void securityChanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    // In case of failure, disconnect
    if (err || (level < BT_SECURITY_L4 && !Cfg.Bt_AllowUnsecuredConnections)) {
        LOG_WRN("Bt security failed: %s, level %u, err %d, disconnecting\n", GetPeerStringByConn(conn), level, err);
        bt_conn_auth_cancel(conn);
        // bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return;
    }


    // Ignore connection that is being paired. At this point, the central is
    // probably talking to us via an anonymous address, and it will yet change.
    if (conn == auth_conn) {
        LOG_INF("Bt connection secured: %s, level %u. It is auth_conn, so ignoring.\n", GetPeerStringByConn(conn), level);
        return;
    }

    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    connection_id_t connectionId = Connections_GetConnectionIdByHostAddr(addr);
    connection_type_t connectionType = Connections_Type(connectionId);
    connectAuthenticatedConnection(conn, connectionId, connectionType);
}

__attribute__((unused)) static void infoLatencyParamsUpdated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    LOG_INF("%s conn params: interval=%u ms, latency=%u, timeout=%u ms\n", GetPeerStringByConn(conn), interval * 5 / 4, latency, timeout * 10);

    bool isUhkPeer = isUhkDeviceConnection(Connections_Type(Peers[GetPeerIdByConn(conn)].connectionId));

    if (interval > 10) {
        configureLatency(conn, isUhkPeer ? LatencyMode_NUS : LatencyMode_BleHid);
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = securityChanged,
    .le_param_updated = infoLatencyParamsUpdated,
};

// Auth callbacks

static void auth_passkey_entry(struct bt_conn *conn) {
    if (auth_conn) {
        bt_conn_disconnect(auth_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }

    auth_conn = bt_conn_ref(conn);

    LOG_INF("Received passkey pairing inquiry.\n");

    if (!auth_conn) {
        LOG_INF("Returning: no auth conn\n");
        return;
    }

    int8_t peerId = GetPeerIdByConn(conn);
    connection_type_t connectionType = Connections_Type(Peers[peerId].connectionId);
    bool isUhkPeer = isUhkDeviceConnection(connectionType);
    if (isUhkPeer || BtPair_OobPairingInProgress) {
        LOG_INF("refusing passkey authentification for %s\n", GetPeerStringByConn(conn));
        bt_conn_auth_cancel(conn);
        return;
    }

#if DEVICE_HAS_OLED
    PairingScreen_AskForPassword();
#endif

    LOG_INF("Type `uhk passkey xxxxxx` to pair, or `uhk passkey -1` to reject\n");
}

static void auth_cancel(struct bt_conn *conn) {
    LOG_INF("Pairing cancelled: peer %s\n", GetPeerStringByConn(conn));

    if (auth_conn) {
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }
}

static void auth_oob_data_request(struct bt_conn *conn, struct bt_conn_oob_info *oob_info) {
    int err;
    struct bt_conn_info info;

    err = bt_conn_get_info(conn, &info);
    if (err) {
        return;
    }


    struct bt_le_oob* oobLocal = BtPair_GetLocalOob();
    struct bt_le_oob* oobRemote = BtPair_GetRemoteOob();

    if (memcmp(info.le.remote->a.val, oobRemote->addr.a.val, sizeof(info.le.remote->a.val))) {
        LOG_WRN("Addresses not matching! Cancelling authentication\n");
        bt_conn_auth_cancel(conn);
        return;
    }

    LOG_INF("Pairing OOB data requested!\n");

    bt_le_oob_set_sc_data(conn, &oobLocal->le_sc_data, &oobRemote->le_sc_data);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_entry = auth_passkey_entry,
    .oob_data_request = auth_oob_data_request,
    .cancel = auth_cancel,
};

// Auth info callbacks

static void pairing_complete(struct bt_conn *conn, bool bonded) {
    LOG_WRN("Pairing completed: %s, bonded %d\n", GetPeerStringByConn(conn), bonded);

    bt_addr_le_t addr = *bt_conn_get_dst(conn);

    if (BtPair_OobPairingInProgress) {
        BtPair_EndPairing(true, "Successfuly bonded!");

        // Disconnect it so that the connection is established only after it is identified as a host connection
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    } else {
        connection_id_t connectionId = Connections_GetConnectionIdByBtAddr(&addr);
        connection_type_t connectionType = Connections_Type(connectionId);

        if (connectionId == ConnectionId_Invalid) {
            connectionId = Connections_GetNewBtHidConnectionId();
            connectionType = ConnectionType_BtHid;
            HostConnection(connectionId)->bleAddress = addr;
            Bt_NewPairedDevice = true;
        }

        // we have to connect from here, because central changes its address *after* setting security
        connectAuthenticatedConnection(conn, connectionId, connectionType);
    }


    if (auth_conn) {
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
        PairingScreen_Feedback(true);
    }

    BtManager_StartScanningAndAdvertisingAsync();
}

static void bt_foreach_conn_cb(struct bt_conn *conn, void *user_data) {
    bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    // gpt says you should unref here. Don't believe it!
}

void BtConn_DisconnectAll() {
    bt_conn_foreach(BT_CONN_TYPE_LE, bt_foreach_conn_cb, NULL);
}

static void bt_foreach_conn_cb_disconnect_unidentified(struct bt_conn *conn, void *user_data) {
    peer_t* peer = getPeerByConn(conn);
    if (!peer) {
    LOG_INF("     disconnecting unassigned connection %s\n", GetPeerStringByConn(conn));
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

void BtConn_DisconnectAllUnidentified() {
    bt_conn_foreach(BT_CONN_TYPE_LE, bt_foreach_conn_cb_disconnect_unidentified, NULL);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
    if (!auth_conn) {
        return;
    }

    if (auth_conn == conn) {
        bt_conn_unref(auth_conn);
        LOG_WRN("Pairing of auth conn failed because of %d\n", reason);
        auth_conn = NULL;
        PairingScreen_Feedback(false);
    }

    LOG_WRN("Pairing failed: %s, reason %d\n", GetPeerStringByConn(conn), reason);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};

void BtConn_Init(void) {
    int err = 0;

    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        Peers[peerId].id = peerId;
        Peers[peerId].conn = NULL;
        Peers[peerId].connectionId = ConnectionId_Invalid;
        strcpy(Peers[peerId].name, "host0");
        Peers[peerId].name[4] = '1' + peerId - PeerIdFirstHost;
    }

    err = bt_conn_auth_cb_register(&conn_auth_callbacks);
    if (err) {
        LOG_WRN("Failed to register authorization callbacks.\n");
    }

    err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
    if (err) {
        LOG_WRN("Failed to register authorization info callbacks.\n");
    }
}

void num_comp_reply(int passkey) {
    struct bt_conn *conn;

#if DEVICE_HAS_OLED
    ScreenManager_SwitchScreenEvent();
#endif

    if (!auth_conn) {
        return;
    }

    conn = auth_conn;

    if (passkey >= 0) {
        bt_conn_auth_passkey_entry(conn, passkey);
        LOG_INF("Sending passkey to conn %s\n", GetPeerStringByConn(conn));
    } else {
        bt_conn_auth_cancel(conn);
        LOG_INF("Reject pairing to conn %s\n", GetPeerStringByConn(conn));
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }
}

uint8_t BtConn_UnusedPeripheralConnectionCount() {
    uint8_t count = MIN(PERIPHERAL_CONNECTION_COUNT, Cfg.Bt_MaxPeripheralConnections);

    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        if (Peers[peerId].conn && count > 0) {
            count--;
        }
    }
    if (auth_conn && count > 0) {
        count--;
    }
    return count;
}

// Unused in left half
ATTR_UNUSED static void disconnectOldestHost() {
    uint32_t oldestSwitchover = UINT32_MAX;
    uint8_t oldestPeerId = PeerIdUnknown;
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        if (Peers[peerId].conn && Peers[peerId].lastSwitchover < oldestSwitchover) {
            oldestSwitchover = Peers[peerId].lastSwitchover;
            oldestPeerId = peerId;
        }
    }

    if (oldestPeerId != PeerIdUnknown) {
        LOG_INF("Disconnecting oldest host %d\n", oldestPeerId);
        bt_conn_disconnect(Peers[oldestPeerId].conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

ATTR_UNUSED static void disconnectAllHids() {
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        if (Peers[peerId].conn && Connections_Type(Peers[peerId].connectionId) == ConnectionType_BtHid) {
            bt_conn_disconnect(Peers[peerId].conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        }
    }
}

void BtConn_ReserveConnections() {
#if DEVICE_IS_UHK80_RIGHT
    bool hostSelected = SelectedHostConnectionId != ConnectionId_Invalid;
    bool hostActive = hostSelected && Connections_IsReady(SelectedHostConnectionId);
    bool selectionIsSatisfied = !hostSelected || hostActive;

    if (!selectionIsSatisfied) {
        // clear filters and restart advertising
        BtAdvertise_Stop();

        BtConn_DisconnectAllUnidentified();

        uint8_t unusedConnectionCount = BtConn_UnusedPeripheralConnectionCount();
        bool selectedConnectionIsBleHid = Connections_Type(SelectedHostConnectionId) == ConnectionType_BtHid;

        if (selectedConnectionIsBleHid && BtConn_ConnectedHidCount() > 0) {
            disconnectAllHids();
            // Advertising will get started when the host actually gets disconnected
        } else if (unusedConnectionCount == 0) {
            disconnectOldestHost();
            // Advertising will get started when the host actually gets disconnected
        } else {
            BtManager_StartScanningAndAdvertising();
        }
        WIDGET_REFRESH(&TargetWidget);
    }
#endif
}

void Bt_SetEnabled(bool enabled) {
    Cfg.Bt_Enabled = enabled;

    if (enabled) {
        LOG_WRN("Starting bluetooth on request.\n");
        BtManager_StartScanningAndAdvertising();
    } else {
        LOG_WRN("Shutting down bluetooth on request!\n");
        BtManager_StopBt();
        BtConn_DisconnectAll();
    }
}

uint8_t BtConn_ConnectedHidCount() {
    uint8_t connectedHids = 0;
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        if (Peers[peerId].conn && Connections_Type(Peers[peerId].connectionId) == ConnectionType_BtHid) {
            connectedHids++;
        }
    }
    return connectedHids;
}

