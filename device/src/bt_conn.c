#include <zephyr/settings/settings.h>
#include "bt_advertise.h"
#include "bt_hid.h"
#include "bt_conn.h"

#define PeerCount 3

#define PeerIdUnknown -1
#define PeerIdLeft 0
#define PeerIdRight 1
#define PeerIdDongle 2

peer_t peers[PeerCount] = {
    {
        .id = PeerIdLeft,
        .addrStr = "e7:f5:5d:7c:82:35",
        .name = "left",
    },
    {
        .id = PeerIdRight,
        .addrStr = "f0:98:11:ce:a4:92",
        .name = "right",
    },
    {
        .id = PeerIdDongle,
        .addrStr = "ac:d6:18:1b:3d:b5",
        .name = "dongle",
    },
};

void InitPeerAddresses(void) {
    for (uint8_t i = 0; i < PeerCount; i++) {
        bt_addr_le_from_str(peers[i].addrStr, "random", &peers[i].addr);
    }
}

peer_t *getPeerByAddr(const bt_addr_le_t *addr) {
    for (uint8_t i = 0; i < PeerCount; i++) {
        if (bt_addr_le_eq(addr, &peers[i].addr)) {
            return &peers[i];
        }
    }

    return NULL;
}

void getPeerIdAndNameByAddr(const bt_addr_le_t *addr, int8_t *peerId, char *peerName) {
    peer_t *peer = getPeerByAddr(addr);

    if (peer) {
        *peerId = peer->id;
        peerName = peer->name;
    } else {
        *peerId = PeerIdUnknown;
        strcpy(peerName, "unknown");
    }
}

static void connected(struct bt_conn *conn, uint8_t err) {
    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    if (err) {
        printk("Failed to connect to conn %p, addr %s, peer, %s err %u\n", conn, addrStr, peerName, err);
        return;
    }

    printk("Connected %s, peer %s\n", addrStr, peerName);

    if (peerId == PeerIdUnknown) {
        err = HidsConnected(conn);

        if (!HidConnection) {
            HidConnection = bt_conn_ref(conn);
            HidInBootMode = false;
            // advertise_hid();
        }
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    printk("Disconnected from conn %p, addr %s, peer %s, reason %u\n", conn, addrStr, peerName, reason);

    if (peerId == PeerIdUnknown) {
        HidsDisconnected(conn);
        HidConnection = NULL;
        advertise_hid();
    }
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    if (!err) {
        printk("Security changed: conn %p, addr %s, peer %s, level %u\n", conn, addrStr, peerName, level);
    } else {
        printk("Security failed: conn %p, addr %s, peer %s, level %u, err %d\n", conn, addrStr, peerName, level, err);
    }
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
    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    printk("Passkey for conn %p, addr %s, peer %s: %06u\n", conn, addrStr, peerName, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey) {
    auth_conn = bt_conn_ref(conn);
    if (!auth_conn) {
        return;
    }

    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    printk("Passkey for conn %p, addr %s, peer %s: %06u\n", conn, addrStr, peerName, passkey);
    printk("Type `uhk btacc 1/0` to accept/reject\n");
}

static void auth_cancel(struct bt_conn *conn) {
    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    printk("Pairing cancelled: conn %p, addr %s, peer %s\n", conn, addrStr, peerName);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
    .cancel = auth_cancel,
};

// Auth info callbacks

static void pairing_complete(struct bt_conn *conn, bool bonded) {
    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    printk("Pairing completed: conn %p, addr %s, peer %s, bonded %d\n", conn, addrStr, peerName, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
    if (!auth_conn) {
        return;
    }

    if (auth_conn == conn) {
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }

    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));

    int8_t peerId;
    char peerName[PeerNameMaxLength];
    getPeerIdAndNameByAddr(addr, &peerId, peerName);

    printk("Pairing failed: conn %p, addr %s, peer %s, reason %d\n", conn, addrStr, peerName, reason);
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
    settings_load();
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
