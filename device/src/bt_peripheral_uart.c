#include <bluetooth/services/nus.h>
#include "bt_conn.h"
#include "bt_advertise.h"
#include "bt_conn.h"

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len) {
    printk("NUS data received from %s: %i\n", GetPeerStringByConn(conn), len);
}

static void bt_send_cb(struct bt_conn *conn) {
    char addr[BT_ADDR_LE_STR_LEN] = {0};
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
    printk("NUS data sent to %s\n", GetPeerStringByConn(conn));
}

static struct bt_nus_cb nus_cb = {
    .received = bt_receive_cb,
    .sent = bt_send_cb,
};

void InitPeripheralUart(void) {
    int err = bt_nus_init(&nus_cb);
    if (err) {
        printk("Failed to initialize UART service (err: %d)\n", err);
        return;
    }

    advertise_peer();
}

void SendPeripheralUart(const uint8_t *data, uint16_t len) {
    int err = bt_nus_send(NULL, data, len);
    if (err) {
        printk("Failed to send data over BLE connection (err: %d)\n", err);
    }
}
