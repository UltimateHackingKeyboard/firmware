#include <stdio.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/scan.h>
#include "bt_advertise.h"
#include "bt_conn.h"
#include "device_state.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/widgets/widget.h"
#include "nus_client.h"
#include "nus_server.h"
#include "device.h"
#include "keyboard/oled/screens/pairing_screen.h"
#include "usb/usb.h"
#include "keyboard/oled/widgets/widgets.h"
#include <zephyr/settings/settings.h>
#include "bt_pair.h"

#define PeerCount 3

#define BLE_KEY_LEN 16
#define BLE_ADDR_LEN 6

peer_t Peers[PeerCount] = {
    {
        .id = PeerIdLeft,
        .name = "left",
    },
    {
        .id = PeerIdRight,
        .name = "right",
    },
    {
        .id = PeerIdDongle,
        .name = "dongle",
    },
};

peer_t *getPeerByAddr(const bt_addr_le_t *addr) {
    for (uint8_t i = 0; i < PeerCount; i++) {
        if (bt_addr_le_eq(addr, &Peers[i].addr)) {
            return &Peers[i];
        }
    }

    return NULL;
}

int8_t getPeerIdByConn(const struct bt_conn *conn) {
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    peer_t *peer = getPeerByAddr(addr);
    int8_t peerId = peer ? peer->id : PeerIdUnknown;
    return peerId;
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
        printk("LE latencies update failed: %d", err);
    }
}

static void connected(struct bt_conn *conn, uint8_t err) {
    int8_t peerId = getPeerIdByConn(conn);

    if (err) {
        printk("Failed to connect to %s, err %u\n", GetPeerStringByConn(conn), err);

        if (DEVICE_IS_UHK80_RIGHT) {
            err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
            if (err) {
                printk("Scanning failed to start (err %d)\n", err);
            }
        }

        return;
    }

    printk("Connected to %s\n", GetPeerStringByConn(conn));

    if (peerId == PeerIdUnknown) {
        if (DEVICE_IS_UHK80_RIGHT) {
            static const struct bt_le_conn_param conn_params = BT_LE_CONN_PARAM_INIT(
                6, 9, // keep it low, lowest allowed is 6 (7.5ms), lowest supported widely is 9 (11.25ms)
                10, // keeping it higher allows power saving on peripheral when there's nothing to send (keep it under 30 though)
                100 // connection timeout (*10ms)
            );
            bt_conn_le_param_update(conn, &conn_params);

            USB_DisableHid();

            DeviceState_SetConnection(ConnectionId_BluetoothHid, ConnectionType_Bt);
        }
    } else {
        bt_conn_set_security(conn, BT_SECURITY_L4);
        // continue connection process in in `connectionSecured()`
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    int8_t peerId = getPeerIdByConn(conn);
    ARG_UNUSED(peerId);

    printk("Disconnected from %s, reason %u\n", GetPeerStringByConn(conn), reason);

    if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
        if (peerId == PeerIdUnknown) {
            Advertise(ADVERTISE_NUS | ADVERTISE_HID);
            if (DEVICE_IS_UHK80_RIGHT) {
                USB_EnableHid();
            }
        } else if (peerId == PeerIdDongle) {
            Advertise(ADVERTISE_NUS);
        } else if (peerId == PeerIdLeft || peerId == PeerIdRight) {
            int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
            printk("Start scan\n");
            if (err) {
                printk("Scanning failed to start (err %d)\n", err);
            }
        }
    }

    if (DEVICE_IS_UHK80_LEFT && peerId == PeerIdRight) {
        NusServer_Disconnected();
    }
    if (DEVICE_IS_UHK80_RIGHT && peerId == PeerIdDongle) {
        NusServer_Disconnected();
    }
    if (DEVICE_IS_UHK80_RIGHT && peerId == PeerIdLeft) {
        NusClient_Disconnected();
    }
    if (DEVICE_IS_UHK_DONGLE && peerId == PeerIdRight) {
        NusClient_Disconnected();
    }

    if (peerId != PeerIdUnknown) {
        Peers[peerId].isConnected = false;
        DeviceState_TriggerUpdate();
    } else {
        DeviceState_SetConnection(ConnectionId_BluetoothHid, ConnectionType_None);
    }
}

bool Bt_DeviceIsConnected(device_id_t deviceId) {
    switch (deviceId) {
        case DeviceId_Uhk80_Left:
            return Peers[PeerIdLeft].isConnected;
        case DeviceId_Uhk80_Right:
            return Peers[PeerIdRight].isConnected;
        case DeviceId_Uhk_Dongle:
            return Peers[PeerIdDongle].isConnected;
        default:
            return false;
    }
}

void Bt_SetDeviceConnected(device_id_t deviceId) {
    switch (deviceId) {
        case DeviceId_Uhk80_Left:
            Peers[PeerIdLeft].isConnected = true;
            break;
        case DeviceId_Uhk80_Right:
            Peers[PeerIdRight].isConnected = true;
            break;
        case DeviceId_Uhk_Dongle:
            Peers[PeerIdDongle].isConnected = true;
            break;
        default:
            break;
    }
    DeviceState_TriggerUpdate();
}

static void securityChanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    int8_t peerId = getPeerIdByConn(conn);

    if (err || (peerId != PeerIdUnknown && level < BT_SECURITY_L4)) {
        printk("Security failed: %s, level %u, err %d, disconnecting\n", GetPeerStringByConn(conn), level, err);
        bt_conn_auth_cancel(conn);
        return;
    }

    printk("Connection secured: %s, level %u, peerId %d\n", GetPeerStringByConn(conn), level, peerId);

    if (peerId != PeerIdUnknown) {
        configureLatency(conn);
        enableDataLengthExtension(conn);

        if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
            NusClient_Connect(conn);
        }
    }

#if DEVICE_IS_UHK80_LEFT
    // gatt_discover(conn); // Taken from bt_central_uart.c
#endif
}

__attribute__((unused)) static void infoLatencyParamsUpdated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    if (getPeerIdByConn(conn) == PeerIdUnknown) {
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
    auth_conn = bt_conn_ref(conn);
    if (!auth_conn) {
        return;
    }

    int8_t peerId = getPeerIdByConn(conn);
    if (peerId != PeerIdUnknown || BtPair_OobPairingInProgress) {
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

void bt_init(void)
{
    int err = 0;

    err = bt_conn_auth_cb_register(&conn_auth_callbacks);
    if (err) {
        printk("Failed to register authorization callbacks.\n");
    }

    err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
    if (err) {
        printk("Failed to register authorization info callbacks.\n");
    }

    bt_enable(NULL);
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
