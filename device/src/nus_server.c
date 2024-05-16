#include "nus_server.h"
#include <bluetooth/services/nus.h>
#include "bt_conn.h"
#include "bt_advertise.h"
#include "bt_conn.h"

static void received(struct bt_conn *conn, const uint8_t *const data, uint16_t len) {
    printk("NUS data received from %s: %i\n", GetPeerStringByConn(conn), len);
}

static void sent(struct bt_conn *conn) {
    char addr[BT_ADDR_LE_STR_LEN] = {0};
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
    printk("NUS data sent to %s\n", GetPeerStringByConn(conn));
}

static struct bt_nus_cb nus_cb = {
    .received = received,
    .sent = sent,
};

void NusServer_Init(void) {
    int err = bt_nus_init(&nus_cb);
    if (err) {
        printk("Failed to initialize UART service (err: %d)\n", err);
        return;
    }

    AdvertiseNus();
}

void NusServer_Send(const uint8_t *data, uint16_t len) {
    int err = bt_nus_send(NULL, data, len);
    if (err) {
        printk("Failed to send data over BLE connection (err: %d)\n", err);
    }
}

void NusServer_SendSyncableProperty(syncable_property_id_t property, const uint8_t *data, uint16_t len) {
    uint8_t buffer[MAX_LINK_PACKET_LENGTH];
    buffer[0] = property;
    memcpy(&buffer[1], data, len);

    NusServer_Send(buffer, len+1);
}
