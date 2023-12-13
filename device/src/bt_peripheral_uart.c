#include <bluetooth/services/nus.h>
#include <zephyr/logging/log.h>
#include "bt_advertise.h"

#define LOG_MODULE_NAME peripheral_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define DEVICE_NAME "UHK 80 Peripheral"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    char addr[BT_ADDR_LE_STR_LEN] = {0};
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
    LOG_INF("Received data from: %s", addr);
}

static struct bt_nus_cb nus_cb = {
    .received = bt_receive_cb,
};

void InitPeripheralUart(void)
{
    int err = bt_nus_init(&nus_cb);
    if (err) {
        LOG_ERR("Failed to initialize UART service (err: %d)", err);
        return;
    }

    advertise_peer();
}

// if (bt_nus_send(NULL, buf->data, buf->len)) {
//     LOG_WRN("Failed to send data over BLE connection");
// }
