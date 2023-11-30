#include "bt_advertise.h"
#include "bt_hid.h"

bt_addr_le_t leftAddr = { .a={ 0xe7, 0xf5, 0x5d, 0x7c, 0x82, 0x35 }, .type=BT_ADDR_LE_RANDOM };
bt_addr_le_t rightAddr = { .a={ 0xf0, 0x98, 0x11, 0xce, 0xa4, 0x92 }, .type=BT_ADDR_LE_RANDOM };
bt_addr_le_t dongleAddr = { .a={ 0x01, 0x23, 0x45, 0x67, 0x89, 0xab }, .type=BT_ADDR_LE_RANDOM };

static void connected(struct bt_conn *conn, uint8_t err) {
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (err) {
        printk("Failed to connect to %s (%u)\n", addr, err);
        return;
    }

    printk("Connected %s\n", addr);
    err = HidsConnected(conn);

    if (!HidConnection) {
        HidConnection = conn;
        HidInBootMode = false;
        advertise_hid();
    }

    printk("Advertising stopped\n");
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
