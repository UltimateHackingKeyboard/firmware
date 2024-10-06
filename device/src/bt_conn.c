#include <stdio.h>
#include <zephyr/settings/settings.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/scan.h>
#include "bt_advertise.h"
#include "bt_conn.h"
#include "device_state.h"
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/widgets/widget.h"
#include "legacy/power_mode.h"
#include "nus_client.h"
#include "nus_server.h"
#include "device.h"
#include "keyboard/oled/screens/pairing_screen.h"
#include "usb/usb.h"
#include "keyboard/oled/widgets/widgets.h"
#include "power_mode.h"

#define PeerCount 3

peer_t Peers[PeerCount] = {
    {
        .id = PeerIdLeft,
        .name = "left",
        .isConnected = false,
        .conn = NULL,
    },
    {
        .id = PeerIdRight,
        .name = "right",
        .isConnected = false,
        .conn = NULL,
    },
    {
        .id = PeerIdDongle,
        .name = "dongle",
        .isConnected = false,
        .conn = NULL,
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

static void set_data_length_extension_params(struct bt_conn *conn) {
    data_len = BT_LE_DATA_LEN_PARAM_MAX;

    int err = bt_conn_le_data_len_update(conn, data_len);
    if (err) {
        printk("LE data length update failed: %d", err);
    }
}

static void set_latency_params(struct bt_conn *conn) {
    static const struct bt_le_conn_param rightActive = BT_LE_CONN_PARAM_INIT(
        6, 6, // keep it low, lowest allowed is 6 (7.5ms), lowest supported widely is 9 (11.25ms)
        10, // keeping it higher allows power saving on peripheral when there's nothing to send (keep it under 30 though)
        300 // connection timeout (*10ms)
    );
    static const struct bt_le_conn_param rightSleep = BT_LE_CONN_PARAM_INIT(
        100, 100,
        10, // so this means a ping every 1.25s?
        300
    );
    static const struct bt_le_conn_param slave = BT_LE_CONN_PARAM_INIT(
        6, 100,
        10,
        300
    );
    int err;
    if (DEVICE_IS_UHK80_RIGHT) {
        err = bt_conn_le_param_update(conn, CurrentPowerMode >= PowerMode_LightSleep ? &rightSleep : &rightActive);
    } else {
        err = bt_conn_le_param_update(conn, &slave);
    }
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
            // https://developer.apple.com/library/archive/qa/qa1931/_index.html
            // https://punchthrough.com/manage-ble-connection/
            // https://devzone.nordicsemi.com/f/nordic-q-a/28058/what-is-connection-parameters
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
        Peers[peerId].conn = bt_conn_ref(conn);

        set_latency_params(conn);

        set_data_length_extension_params(conn);

        if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
            NusClient_Setup(conn);
        }
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
        bt_conn_unref(Peers[peerId].conn);
        Peers[peerId].isConnected = false;
        DeviceState_TriggerUpdate();
    } else {
        DeviceState_SetConnection(ConnectionId_BluetoothHid, ConnectionType_None);
    }
}

void Bt_UpdatePowerModes() {
    if (DEVICE_IS_UHK80_RIGHT) {
        for (uint8_t peerId = 0; peerId < PeerCount; peerId++) {
            if (Peers[peerId].isConnected && Peers[peerId].conn) {
                set_latency_params(Peers[peerId].conn);
            }
        }
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

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    if (!err) {
        printk("Security changed: %s, level %u\n", GetPeerStringByConn(conn), level);
    } else {
        printk("Security failed: %s, level %u, err %d\n", GetPeerStringByConn(conn), level, err);
    }

#if DEVICE_IS_UHK80_LEFT
    // gatt_discover(conn); // Taken from bt_central_uart.c
#endif
}

__attribute__((unused)) static void le_param_updated(struct bt_conn* conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    if (getPeerIdByConn(conn) == PeerIdUnknown) {
        printk("BLE HID conn params: interval=%u ms, latency=%u, timeout=%u ms\n",
            interval * 5 / 4, latency, timeout * 10);
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
    .le_param_updated = le_param_updated,
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

#if DEVICE_HAS_OLED
    PairingScreen_AskForPassword(passkey);
#endif

    printk("Passkey for %s: %06u\n", GetPeerStringByConn(conn), passkey);
    printk("Type `uhk btacc 1/0` to accept/reject\n");
}

static void auth_cancel(struct bt_conn *conn) {
    printk("Pairing cancelled: peer %s\n", GetPeerStringByConn(conn));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
    .cancel = auth_cancel,
};

// Auth info callbacks

static void pairing_complete(struct bt_conn *conn, bool bonded) {
    printk("Pairing completed: %s, bonded %d\n", GetPeerStringByConn(conn), bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
    if (!auth_conn) {
        return;
    }

    if (auth_conn == conn) {
        bt_conn_unref(auth_conn);
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
