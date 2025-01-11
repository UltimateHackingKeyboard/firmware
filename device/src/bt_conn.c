#include "keyboard/oled/framebuffer.h"
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

bool Bt_NewPairedDevice = false;

struct bt_conn *auth_conn;

#define BLE_KEY_LEN 16
#define BLE_ADDR_LEN 6

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
    char addrStr[BT_ADDR_STR_LEN];
    for (uint8_t i=0; i<BT_ADDR_SIZE; i++) {
        sprintf(&addrStr[i*3], "%02x:", addr->a.val[BT_ADDR_SIZE-1-i]);
    }
    addrStr[BT_ADDR_STR_LEN-1] = '\0';

    peer_t *peer = getPeerByAddr(addr);
    char peerName[PeerNameMaxLength];

    if (peer) {
        strcpy(peerName, peer->name);
    } else {
        strcpy(peerName, "unknown");
    }

    static char peerString[PeerNameMaxLength + BT_ADDR_LE_STR_LEN + 3];
    sprintf(peerString, "%s (%s)", peerName, addrStr);

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
        printk("LE data length update failed: %d", err);
    }
}

static void setLatency(struct bt_conn* conn, const struct bt_le_conn_param* params) {
    int err = bt_conn_le_param_update(conn, params);
    if (err) {
        printk("LE latencies update failed: %d\n", err);
    }
}

static void configureLatency(struct bt_conn *conn, latency_mode_t latencyMode) {
    switch (latencyMode) {
        case LatencyMode_NUS:
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

void BtConn_UpdateHostConnectionPeerAllocations() {
    //for each peer
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        struct bt_conn* conn = Peers[peerId].conn;
        if (conn) {
            connection_id_t currentId = Peers[peerId].connectionId;
            connection_id_t newId = Connections_GetConnectionIdByHostAddr(bt_conn_get_dst(conn));
            printk("Reallocating peer %s from connection %d -> %d\n", Peers[peerId].name, currentId, newId);
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
        printk("No peer slot available for connection %d\n", connectionId);
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
        printk("  - %s\n", GetPeerStringByConn(conn));
    } else {
        printk("  - peer %d(%s), connection %d\n", peerId, GetPeerStringByConn(conn), Peers[peerId].connectionId);
    }
}

void BtConn_ListCurrentConnections() {
    printk("Current connections:\n");
    bt_conn_foreach(BT_CONN_TYPE_LE, bt_foreach_list_current_connections, NULL);
}


static void bt_foreach_print_bond(const struct bt_bond_info *info, void *user_data)
{
    printk(" - %s\n", GetAddrString(&info->addr));
}

void BtConn_ListAllBonds() {
    printk("All bonds:\n");
    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_print_bond, NULL);
}


// If last available slot is reserved for a selected connection, refuse other connections
static bool isWanted(struct bt_conn *conn, connection_type_t connectionType) {
    return
        connectionType == ConnectionType_NusLeft ||
        BtConn_UnusedPeripheralConnectionCount() > 1 ||
        SelectedHostConnectionId == ConnectionId_Invalid ||
        BtAddrEq(bt_conn_get_dst(conn), &HostConnection(SelectedHostConnectionId)->bleAddress);
}

static void connectNus(struct bt_conn *conn, connection_id_t connectionId, connection_type_t connectionType) {
    uint8_t peerId = assignPeer(conn, connectionId, connectionType);

    printf("Bt connected to %s\n", GetPeerStringByConn(conn));

    configureLatency(conn, LatencyMode_NUS);
    enableDataLengthExtension(conn);

    bool isRightClient = DEVICE_IS_UHK80_RIGHT && peerId == PeerIdLeft;
    bool isDongleClient = DEVICE_IS_UHK_DONGLE && peerId == PeerIdRight;
    if ( isRightClient || isDongleClient ) {
        printk("Initiating NUS connection with %s\n", GetPeerStringByConn(conn));
        NusClient_Connect(conn);
    }
}

static void connectHid(struct bt_conn *conn, connection_id_t connectionId, connection_type_t connectionType) {
    assignPeer(conn, connectionId, connectionType);

    printf("Bt connected to %s\n", GetPeerStringByConn(conn));

    configureLatency(conn, LatencyMode_NUS);

    // Assume that HOGP is ready
    printf("Established HID connection with %s\n", GetPeerStringByConn(conn));
    Connections_SetState(connectionId, ConnectionState_Ready);
}

#define BT_UUID_NUS_VAL BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_NUS BT_UUID_DECLARE_128(BT_UUID_NUS_VAL)

ATTR_UNUSED static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params)
{
    if (!attr) {
        printk("Service discovery completed, connection wasn't matched.\n");
        // TODO: consider setting a timer to disconnect the connection if neither auth nor security is Established
        return BT_GATT_ITER_STOP;
    }

    if (attr->user_data) {
        struct bt_gatt_service_val *service_val = (struct bt_gatt_service_val *)attr->user_data;
        if (service_val && service_val->uuid) {
            if (service_val->uuid->type == BT_UUID_TYPE_128 && !bt_uuid_cmp(service_val->uuid, BT_UUID_NUS)) {
                if (!BtPair_OobPairingInProgress && DEVICE_IS_UHK80_RIGHT) {
                    printk("Unknown NUS trying to connect. Refusing!\n");
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

    printk("Bt connected to unknown. Starting discovery.\n");
    static struct bt_gatt_discover_params discover_params;
    discover_params.uuid = NULL;  // Will discover all services
    discover_params.start_handle = 0x0001;
    discover_params.end_handle = 0xFFFF;
    discover_params.func = discover_func;
    discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    err = bt_gatt_discover(conn, &discover_params);
    if (err) {
        printk("Service discovery failed (err %u)\n", err);
        return;
    }
#endif
}

static void connected(struct bt_conn *conn, uint8_t err) {
    if (err) {
        printk("Failed to connect to %s, err %u\n", GetPeerStringByConn(conn), err);
        BtManager_StartScanningAndAdvertising();
        return;
    }

    const bt_addr_le_t * addr = bt_conn_get_dst(conn);
    connection_id_t connectionId = Connections_GetConnectionIdByHostAddr(addr);
    connection_type_t connectionType = Connections_Type(connectionId);

    printk("connected %s, %d %d\n", GetPeerStringByConn(conn), connectionId, connectionType);

    if (connectionId == ConnectionId_Invalid) {
        connectUnknown(conn);
    } else {

        if (isWanted(conn, connectionType)) {
            bt_conn_set_security(conn, BT_SECURITY_L4);
        } else {
            printk("Refusing connenction %d (this is not a selected connection)\n", connectionId);
            bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        }
    }

    BtManager_StartScanningAndAdvertisingAsync();


    return;

    /*
     *+        switch (connectionType) {
+            case ConnectionType_NusLeft:
+            case ConnectionType_NusRight:
+            case ConnectionType_NusDongle:
+                connectNus(conn, connectionId, connectionType);
+                break;
+            case ConnectionType_BtHid:
+                connectHid(conn, connectionId, connectionType);
+                break;
+            case ConnectionType_Unknown:
+            default:
+                connectedUnknown(conn);
+                break;
+        }
+
+        if (DEVICE_IS_UHK80_RIGHT) {
+            BtManager_StartScanningAndAdvertising();
+        }
*/

/*
    const bt_addr_le_t * addr = bt_conn_get_dst(conn);
    connection_id_t connectionId = Connections_GetConnectionIdByBtAddr(addr);
    connection_type_t connectionType = Connections_Type(connectionId);

    if (err) {
        printk("Failed to connect to %s, err %u\n", GetPeerStringByConn(conn), err);
        BtManager_StartScanningAndAdvertising();
        return;
    }

    // If last available slot is reserved for a selected connection, refuse other connections
    if (
            connectionType != ConnectionType_NusLeft &&
            BtConn_UnusedPeripheralConnectionCount() <= 1 &&
            SelectedHostConnectionId != ConnectionId_Invalid &&
            !BtAddrEq(bt_conn_get_dst(conn), &HostConnection(SelectedHostConnectionId)->bleAddress)
    ) {
        printk("Refusing connenction %d (this is not a selected connection)\n", connectionId);
        err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        return;
    }

    if (connectionType == ConnectionType_BtHid || connectionType == ConnectionType_Unknown) {
        static const struct bt_le_conn_param conn_params = BT_LE_CONN_PARAM_INIT(
                6, 9, // keep it low, lowest allowed is 6 (7.5ms), lowest supported widely is 9 (11.25ms)
                10, // keeping it higher allows power saving on peripheral when there's nothing to send (keep it under 30 though)
                100 // connection timeout (*10ms)
                );
        bt_conn_le_param_update(conn, &conn_params);
    }

    if (connectionType == ConnectionType_Unknown) {
        printk("Bt connected to UNKNOWN %s\n", GetPeerStringByConn(conn));
        return;
    }

    assignPeer(conn, connectionId, connectionType);

    printk("Bt connected to %s\n", GetPeerStringByConn(conn));

    if (connectionType == ConnectionType_BtHid) {
    } else {
        bt_conn_set_security(conn, BT_SECURITY_L4);
        // continue connection process in in `connectionSecured()`
    }

    if (DEVICE_IS_UHK80_RIGHT) {
        BtManager_StartScanningAndAdvertising();
    }
    */
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    int8_t peerId = GetPeerIdByConn(conn);
    connection_type_t connectionType = Connections_Type(Peers[peerId].connectionId);

    ARG_UNUSED(peerId);

    printk("Bt disconnected from %s, reason %u\n", GetPeerStringByConn(conn), reason);

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
    if (!isWanted(conn, connectionType)) {
        printk("Refusing connenction %d (this is not a selected connection)\n", connectionId);
        bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
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
            printk("Authenticated connection is not known. Disconnecting %s", GetPeerStringByConn(conn));
            bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            break;
    }
}

static void securityChanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    // In case of failure, disconnect
    if (err || (level < BT_SECURITY_L4 && !Cfg.AllowUnsecuredConnections)) {
        printk("Bt security failed: %s, level %u, err %d, disconnecting\n", GetPeerStringByConn(conn), level, err);
        bt_conn_auth_cancel(conn);
        return;
    }


    // Ignore connection that is being paired. At this point, the central is
    // probably talking to us via an anonymous address, and it will yet change.
    if (conn == auth_conn) {
        printk("Bt connection secured: %s, level %u. It is auth_conn, so ignoring.\n", GetPeerStringByConn(conn), level);
        return;
    }

    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    connection_id_t connectionId = Connections_GetConnectionIdByHostAddr(addr);
    connection_type_t connectionType = Connections_Type(connectionId);
    connectAuthenticatedConnection(conn, connectionId, connectionType);
}


/*
static void securityChanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    int8_t peerId = GetPeerIdByConn(conn);
    uint8_t connectionId;
    uint8_t connectionType;

    if (peerId == PeerIdUnknown) {
        connectionId = Connections_GetConnectionIdByBtAddr(bt_conn_get_dst(conn));
        connectionType = Connections_Type(connectionId);

        if (connectionType == ConnectionType_BtHid) {
            connectHid(conn, connectionId, connectionType);
        }

        if (connectionId == ConnectionId_Invalid && conn != auth_conn) {
            printk("Unknown and non-autheticating connection secured. Disconnecting %s\n", GetPeerStringByConn(conn));
            bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
            return;
        }
    } else {
        connectionId = Peers[peerId].connectionId;
        connectionType = Connections_Type(connectionId);
    }

    bool isUhkPeer = isUhkDeviceConnection(connectionType);
    if (err || (isUhkPeer && level < BT_SECURITY_L4 && !Cfg.AllowUnsecuredConnections)) {
        printk("Bt security failed: %s, level %u, err %d, disconnecting\n", GetPeerStringByConn(conn), level, err);
        bt_conn_auth_cancel(conn);
        return;
    }

    printk("Bt connection secured: %s, level %u, peerId %d\n", GetPeerStringByConn(conn), level, peerId);

    if (isUhkPeer) {
        configureLatency(conn);
        enableDataLengthExtension(conn);

        if (
                (DEVICE_IS_UHK80_RIGHT && peerId == PeerIdLeft)
                || (DEVICE_IS_UHK_DONGLE && peerId == PeerIdRight)
        ) {
            printk("Initiating NUS connection with %s\n", GetPeerStringByConn(conn));
            NusClient_Connect(conn);
        }
    }

#if DEVICE_IS_UHK80_LEFT
    // gatt_discover(conn); // Taken from bt_central_uart.c
#endif
}
*/

__attribute__((unused)) static void infoLatencyParamsUpdated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    uint8_t peerId = GetPeerIdByConn(conn);
    connection_type_t connectionType = Connections_Type(Peers[peerId].connectionId);

    if (connectionType == ConnectionType_BtHid || connectionType == ConnectionType_Unknown) {
        printk("BLE HID conn params: interval=%u ms, latency=%u, timeout=%u ms\n",
            interval * 5 / 4, latency, timeout * 10);
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = securityChanged,
    .le_param_updated = infoLatencyParamsUpdated,
};

// Auth callbacks

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    printk("Passkey for %s: %06u\n", GetPeerStringByConn(conn), passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey) {
    if (auth_conn) {
        bt_conn_disconnect(auth_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }

    auth_conn = bt_conn_ref(conn);

    printk("Received passkey pairing inquiry.\n");

    if (!auth_conn) {
        printk("Returning: no auth conn\n");
        return;
    }

    int8_t peerId = GetPeerIdByConn(conn);
    connection_type_t connectionType = Connections_Type(Peers[peerId].connectionId);
    bool isUhkPeer = isUhkDeviceConnection(connectionType);
    if (isUhkPeer || BtPair_OobPairingInProgress) {
        printk("refusing passkey authentification for %s\n", GetPeerStringByConn(conn));
        bt_conn_auth_cancel(conn);
        return;
    }

#if DEVICE_HAS_OLED
    PairingScreen_AskForPassword(passkey);
#endif

    printk("Passkey for %s: %06u\n", GetPeerStringByConn(conn), passkey);
    printk("Type `uhk btacc 1/0` to accept/reject\n");
}

static void auth_cancel(struct bt_conn *conn) {
    printk("Pairing cancelled: peer %s\n", GetPeerStringByConn(conn));

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
        printk("Addresses not matching! Cancelling authentication\n");
        bt_conn_auth_cancel(conn);
        return;
    }

    printk("Pairing OOB data requested!\n");

    bt_le_oob_set_sc_data(conn, &oobLocal->le_sc_data, &oobRemote->le_sc_data);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
    .oob_data_request = auth_oob_data_request,
    .cancel = auth_cancel,
};

// Auth info callbacks

static void pairing_complete(struct bt_conn *conn, bool bonded) {
    printk("Pairing completed: %s, bonded %d\n", GetPeerStringByConn(conn), bonded);

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

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
    if (!auth_conn) {
        return;
    }

    if (auth_conn == conn) {
        bt_conn_unref(auth_conn);
        printk("Pairing of auth conn failed because of %d\n", reason);
        auth_conn = NULL;
    }

    printk("Pairing failed: %s, reason %d\n", GetPeerStringByConn(conn), reason);
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
        printk("Failed to register authorization callbacks.\n");
    }

    err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
    if (err) {
        printk("Failed to register authorization info callbacks.\n");
    }
}

void num_comp_reply(uint8_t accept) {
    struct bt_conn *conn;

#if DEVICE_HAS_OLED
    ScreenManager_SwitchScreenEvent();
#endif

    if (!auth_conn) {
        return;
    }

    conn = auth_conn;

    if (accept) {
        bt_conn_auth_passkey_confirm(conn);
        printk("Numeric Match, conn %p\n", conn);
    } else {
        bt_conn_auth_cancel(conn);
        printk("Numeric Reject, conn %p\n", conn);
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }
}

uint8_t BtConn_UnusedPeripheralConnectionCount() {
    uint8_t count = 0;
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        if (!Peers[peerId].conn) {
            count++;
        }
    }
    if (auth_conn) {
        count--;
    }
    return count;
}

static void disconnectOldestHost() {
    uint32_t oldestSwitchover = UINT32_MAX;
    uint8_t oldestPeerId = PeerIdUnknown;
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        if (Peers[peerId].conn && Peers[peerId].lastSwitchover < oldestSwitchover) {
            oldestSwitchover = Peers[peerId].lastSwitchover;
            oldestPeerId = peerId;
        }
    }

    if (oldestPeerId != PeerIdUnknown) {
        printk("Disconnecting oldest host %d\n", oldestPeerId);
        bt_conn_disconnect(Peers[oldestPeerId].conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

void BtConn_ReserveConnections() {
    bool hostSelected = SelectedHostConnectionId != ConnectionId_Invalid;
    bool hostActive = hostSelected && Connections_IsReady(SelectedHostConnectionId);
    bool selectionIsSatisfied = !hostSelected || hostActive;
    uint8_t unusedConnectionCount = BtConn_UnusedPeripheralConnectionCount();

    if (!selectionIsSatisfied) {
        // clear filters and restart advertising
        BtAdvertise_Stop();
        if (unusedConnectionCount == 0) {
            disconnectOldestHost();
            // Advertising will get started when the host actually gets disconnected
        } else {
            BtManager_StartScanningAndAdvertising();
        }
    }
}

