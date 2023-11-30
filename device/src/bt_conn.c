#include "bt_advertise.h"
#include "bt_hid.h"

char leftAddrStr[] = "e7:f5:5d:7c:82:35";
char rightAddrStr[] = "f0:98:11:ce:a4:92";
char dongleAddrStr[] = "ac:d6:18:1b:3d:b5";

bt_addr_le_t leftAddr;
bt_addr_le_t rightAddr;
bt_addr_le_t dongleAddr;

void InitAddresses(void) {
    bt_addr_le_from_str(leftAddrStr, "random", &leftAddr);
    bt_addr_le_from_str(rightAddrStr, "random", &rightAddr);
    bt_addr_le_from_str(dongleAddrStr, "public", &dongleAddr);
}

static void connected(struct bt_conn *conn, uint8_t err) {
    char addrStr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addrStr, sizeof(addrStr));
    bt_addr_le_t *addr = bt_conn_get_dst(conn);
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
