#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <soc.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>

#include <bluetooth/services/nus.h>

#include <zephyr/settings/settings.h>

#include <stdio.h>

#include <zephyr/logging/log.h>
#include "bt_advertise.h"

#define LOG_MODULE_NAME peripheral_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#define STACKSIZE CONFIG_BT_NUS_THREAD_STACK_SIZE
#define PRIORITY 7

#define DEVICE_NAME "UHK 80 Peripheral"
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static struct bt_conn *current_conn;
static struct bt_conn *auth_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
    printk("Connected (bt_peripheral_uart)");
    char addr[BT_ADDR_LE_STR_LEN];

    if (err) {
        LOG_ERR("Connection failed (err %u)", err);
        return;
    }

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Connected %s", addr);

    current_conn = bt_conn_ref(conn);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Disconnected: %s (reason %u)", addr, reason);

    if (auth_conn) {
        bt_conn_unref(auth_conn);
        auth_conn = NULL;
    }

    if (current_conn) {
        bt_conn_unref(current_conn);
        current_conn = NULL;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected    = connected,
    .disconnected = disconnected,
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];

    auth_conn = bt_conn_ref(conn);

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    LOG_INF("Passkey for %s: %06u", addr, passkey);
    LOG_INF("Press Button 1 to confirm, Button 2 to reject.");
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Pairing failed conn: %s, reason %d", addr, reason);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .passkey_confirm = auth_passkey_confirm,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_failed = pairing_failed
};

static void bt_receive_cb(struct bt_conn *conn, const uint8_t *const data, uint16_t len)
{
    char addr[BT_ADDR_LE_STR_LEN] = {0};
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, ARRAY_SIZE(addr));
    LOG_INF("Received data from: %s", addr);
}

static struct bt_nus_cb nus_cb = {
    .received = bt_receive_cb,
};

static void num_comp_reply(bool accept)
{
    if (accept) {
        bt_conn_auth_passkey_confirm(auth_conn);
        LOG_INF("Numeric Match, conn %p", (void *)auth_conn);
    } else {
        bt_conn_auth_cancel(auth_conn);
        LOG_INF("Numeric Reject, conn %p", (void *)auth_conn);
    }

    bt_conn_unref(auth_conn);
    auth_conn = NULL;
}

// void button_changed(uint32_t button_state, uint32_t has_changed)
// {
//     uint32_t buttons = button_state & has_changed;

//     if (auth_conn) {
//         if (buttons & KEY_PASSKEY_ACCEPT) {
//             num_comp_reply(true);
//         }

//         if (buttons & KEY_PASSKEY_REJECT) {
//             num_comp_reply(false);
//         }
//     }
// }

int InitPeripheralUart(void)
{
    printk("InitPeripheralUart\n");
    int err = 0;

    // err = bt_conn_auth_cb_register(&conn_auth_callbacks);
    // if (err) {
    //     printk("Failed to register authorization callbacks.\n");
    //     return 0;
    // }

    // err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
    // if (err) {
    //     printk("Failed to register authorization info callbacks.\n");
    //     return 0;
    // }

    err = bt_nus_init(&nus_cb);
    if (err) {
        LOG_ERR("Failed to initialize UART service (err: %d)", err);
        return 0;
    }

    advertise_peer();
}

// if (bt_nus_send(NULL, buf->data, buf->len)) {
//     LOG_WRN("Failed to send data over BLE connection");
// }
