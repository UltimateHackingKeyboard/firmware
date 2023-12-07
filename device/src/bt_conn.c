#include <zephyr/settings/settings.h>
#include "bt_advertise.h"
#include "bt_hid.h"
#include "bt_conn.h"

#define PeerCount 3

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

bt_addr_le_t leftAddr;
bt_addr_le_t rightAddr;
bt_addr_le_t dongleAddr;

void InitAddresses(void) {
    // bt_addr_le_from_str(leftAddrStr, "random", &leftAddr);
    // bt_addr_le_from_str(rightAddrStr, "random", &rightAddr);
    // bt_addr_le_from_str(dongleAddrStr, "public", &dongleAddr);
}

static void connected(struct bt_conn *conn, uint8_t err) {
    char addrStr[BT_ADDR_LE_STR_LEN];
    const bt_addr_le_t *addr = bt_conn_get_dst(conn);
    bt_addr_le_to_str(addr, addrStr, sizeof(addrStr));
    printk("%i", bt_addr_le_eq(addr, &dongleAddr));

    if (err) {
        printk("Failed to connect to %s (%u)\n", addrStr, err);
        return;
    }

    printk("Connected %s\n", addrStr);
    err = HidsConnected(conn);

    if (!HidConnection) {
        HidConnection = bt_conn_ref(conn);
        HidInBootMode = false;
        // advertise_hid();
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Disconnected from %s (reason %u)\n", addr, reason);
    HidsDisconnected(conn);
    HidConnection = NULL;
    advertise_hid();
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (!err) {
        printk("Security changed: %s level %u\n", addr, level);
    } else {
        printk("Security failed: %s level %u err %d\n", addr, level, err);
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
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey) {
    auth_conn = bt_conn_ref(conn);
    if (!auth_conn) {
        return;
    }

    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(auth_conn), addr, sizeof(addr));
    printk("Passkey for %s: %06u\n", addr, passkey);
    printk("type `uhk btacc 1/0` to accept/reject.\n");
}

static void auth_cancel(struct bt_conn *conn) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
    .cancel = auth_cancel,
};

// Auth info callbacks

static void pairing_complete(struct bt_conn *conn, bool bonded) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing completed: %s, bonded: %d\n", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
    if (!auth_conn) {
        return;
    }

    if (auth_conn == conn) {
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }

    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    printk("Pairing failed conn: %s, reason %d\n", addr, reason);
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
