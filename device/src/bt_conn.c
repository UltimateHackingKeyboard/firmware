#include <stdio.h>
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

bool Bt_NewPairedDevice = false;

#define BLE_KEY_LEN 16
#define BLE_ADDR_LEN 6

peer_t Peers[PeerCount] = {
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
    for (uint8_t i = 0; i < PeerCount; i++) {
        if (BtAddrEq(addr, &Peers[i].addr)) {
            return &Peers[i];
        }
    }

    return NULL;
}

peer_t *getPeerByConn(const struct bt_conn *conn) {
    for (uint8_t i = 0; i < PeerCount; i++) {
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

static void configureLatency(struct bt_conn *conn) {
    // https://developer.apple.com/library/archive/qa/qa1931/_index.html
    // https://punchthrough.com/manage-ble-connection/
    // https://devzone.nordicsemi.com/f/nordic-q-a/28058/what-is-connection-parameters
    static const struct bt_le_conn_param conn_params = BT_LE_CONN_PARAM_INIT(
        6, 9, // keep it low, lowest allowed is 6 (7.5ms), lowest supported widely is 9 (11.25ms)
        0, // keeping it higher allows power saving on peripheral when there's nothing to send (keep it under 30 though)
        100 // connection timeout (*10ms)
    );
    int err = bt_conn_le_param_update(conn, &conn_params);
    if (err) {
        printk("LE latencies update failed: %d\n", err);
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
    return PeerIdLastHost;
}

static void assignPeer(struct bt_conn* conn, uint8_t connectionId, uint8_t connectionType) {
    uint8_t peerId = allocateHostPeer(connectionType);
    bt_addr_le_t addr = *bt_conn_get_dst(conn);
    if (Peers[peerId].conn) {
        printk("Peer slot %d already occupied!\n", peerId);
    }
    printk("Allocating connectionId %d to peer %d\n", connectionId, peerId);
    Peers[peerId].addr = addr;
    Peers[peerId].conn = bt_conn_ref(conn);
    Peers[peerId].connectionId = connectionId;
    Connections[connectionId].peerId = peerId;
    Connections_SetState(connectionId, ConnectionState_Connected);
}

void check_connection(struct bt_conn *conn, void *data)
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
    bt_conn_foreach(BT_CONN_TYPE_LE, check_connection, NULL);
}


static void bt_foreach_print_bond(const struct bt_bond_info *info, void *user_data)
{
    printk(" - %s\n", GetAddrString(&info->addr));
}

void BtConn_ListAllBonds() {
    printk("All bonds:\n");
    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_print_bond, NULL);
}

static void connected(struct bt_conn *conn, uint8_t err) {
    const bt_addr_le_t * addr = bt_conn_get_dst(conn);
    connection_id_t connectionId = Connections_GetConnectionIdByBtAddr(addr);
    connection_type_t connectionType = Connections_Type(connectionId);

    if (err) {
        printk("Failed to connect to %s, err %u\n", GetPeerStringByConn(conn), err);

#if DEVICE_IS_UHK80_RIGHT
        err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
        if (err) {
            printk("Scanning failed to start (err %d)\n", err);
        }
#endif

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
#if DEVICE_IS_UHK80_RIGHT
        USB_DisableHid();
#endif
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
        BtAdvertise_Start(BtAdvertise_Type());
    }
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
        if (DEVICE_IS_UHK80_RIGHT) {
            if (connectionType == ConnectionType_BtHid) {
                BtAdvertise_Stop();
                BtAdvertise_Start(BtAdvertise_Type());
                USB_EnableHid();
            }
            if (connectionType == ConnectionType_NusDongle) {
                BtAdvertise_Stop();
                BtAdvertise_Start(BtAdvertise_Type());
            }
            if (connectionType == ConnectionType_NusLeft) {
                BtScan_Start();
            }
        }

        if (DEVICE_IS_UHK_DONGLE && connectionType == ConnectionType_NusRight) {
            // give other dongles a chance to connect
            EventScheduler_Schedule(k_uptime_get_32()+50, EventSchedulerEvent_BtStartScanning, "disconnected");
        }

        if (DEVICE_IS_UHK80_LEFT && connectionType == ConnectionType_NusRight) {
            BtAdvertise_Start(BtAdvertise_Type());
        }
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

static void securityChanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    int8_t peerId = GetPeerIdByConn(conn);
    uint8_t connectionId = Peers[peerId].connectionId;
    connection_type_t connectionType = Connections_Type(connectionId);

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

struct bt_conn *auth_conn;

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    printk("Passkey for %s: %06u\n", GetPeerStringByConn(conn), passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey) {
    if (auth_conn) {
        bt_conn_unref(auth_conn);
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
    BtPair_EndPairing(true, "Successfuly bonded!");

    uint8_t peerId = GetPeerIdByConn(conn);

    // address can change during pairing
    bt_addr_le_t addr = *bt_conn_get_dst(conn);
    Peers[peerId].addr = addr;

    connection_type_t connectionType = Connections_Type(Peers[peerId].connectionId);
    bool isUhkPeer = isUhkDeviceConnection(connectionType);

    bool isKnown = HostConnections_IsKnownBleAddress(&addr);
    printk("- is known: %d, isUhkPeer: %d\n", isKnown, isUhkPeer);
    if (!isKnown && !isUhkPeer) {
        printk("setting NewPairedDevice\n");
        Bt_NewPairedDevice = true;
    }

    BtConn_ListCurrentConnections();
    BtConn_ListAllBonds();
    HostConnections_ListKnownBleConnections();
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
    }

    bt_conn_unref(auth_conn);
    auth_conn = NULL;
}

uint8_t BtConn_UnusedPeripheralConnectionCount() {
    uint8_t count = 0;
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        if (!Peers[peerId].conn) {
            count++;
        }
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
            BtAdvertise_Start(ADVERTISE_HID | ADVERTISE_NUS);
        }
    }
}

