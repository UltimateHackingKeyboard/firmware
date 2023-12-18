#include <stdio.h>
#include <zephyr/settings/settings.h>
#include <bluetooth/scan.h>
#include "bt_advertise.h"
#include "bt_hid.h"
#include "bt_conn.h"
#include "bt_central_uart.h"
#include "device.h"

#define PeerCount 3

#define PeerIdUnknown -1
#define PeerIdLeft 0
#define PeerIdRight 1
#define PeerIdDongle 2

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

void getPeerIdAndNameByAddr(const bt_addr_le_t *addr, int8_t *peerId, char *peerName) {
    peer_t *peer = getPeerByAddr(addr);

    if (peer) {
        *peerId = peer->id;
        strcpy(peerName, peer->name);
    } else {
        *peerId = PeerIdUnknown;
        strcpy(peerName, "unknown");
    }
}

uint8_t getPeerIdByConn(const struct bt_conn *conn) {
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);
    return peerId;
}

char *GetPeerStringByConn(const struct bt_conn *conn) {
    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    static char peerString[PeerNameMaxLength + BT_ADDR_LE_STR_LEN + 3];
    sprintf(peerString, "%s (%s)", peerName, addrStr);

    return peerString;
}

static struct bt_conn *current_conn;

static void connected(struct bt_conn *conn, uint8_t err) {
    int8_t peerId = getPeerIdByConn(conn);

    if (err) {
        printk("Failed to connect to %s, err %u\n", GetPeerStringByConn(conn), err);

        if (current_conn == conn) {
            bt_conn_unref(current_conn);
            current_conn = NULL;

#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
            err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
            if (err) {
                printk("Scanning failed to start (err %d)", err);
            }
#endif
        }

        return;
    }

    printk("Connected to %s\n", GetPeerStringByConn(conn));

    if (peerId == PeerIdUnknown) {
        err = HidsConnected(conn);

        if (!HidConnection) {
            HidConnection = bt_conn_ref(conn);
            HidInBootMode = false;
            // advertise_hid();
        }
    } else {
        current_conn = bt_conn_ref(conn);
#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
    SetupCentralConnection(conn);
#endif
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    int8_t peerId = getPeerIdByConn(conn);

    printk("Disconnected from %s, reason %u\n", GetPeerStringByConn(conn), reason);

#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_RIGHT
    if (peerId == PeerIdUnknown) {
        HidsDisconnected(conn);
        HidConnection = NULL;
        advertise_hid();
    } else {
        // if (auth_conn) {
        //     bt_conn_unref(auth_conn);
        //     auth_conn = NULL;
        // }

        if (current_conn) {
            bt_conn_unref(current_conn);
            current_conn = NULL;
        }

        int err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
        printk("Start scan\n");
        if (err) {
            printk("Scanning failed to start (err %d)", err);
        }
    }
#elif CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
    if (current_conn != conn) {
        return;
    }

    bt_conn_unref(current_conn);
    current_conn = NULL;
#endif
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    if (!err) {
        printk("Security changed: %s, level %u\n", GetPeerStringByConn(conn), level);
    } else {
        printk("Security failed: %s, level %u, err %d\n", GetPeerStringByConn(conn), level, err);
    }

#if CONFIG_DEVICE_ID == DEVICE_ID_UHK80_LEFT
    gatt_discover(conn); // Taken from bt_central_uart.c
#endif
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
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
