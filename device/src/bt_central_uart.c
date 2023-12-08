#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/scan.h>
#include <zephyr/logging/log.h>
#include "bt_scan.h"

#define LOG_MODULE_NAME central_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static struct bt_nus_client nus_client;

static void ble_data_sent(struct bt_nus_client *nus, uint8_t err, const uint8_t *const data, uint16_t len)
{
    ARG_UNUSED(nus);
    printk("BLE Data sent: %s\n", data);
    if (err) {
        LOG_WRN("ATT error code: 0x%02X", err);
    }
}

static uint8_t ble_data_received(struct bt_nus_client *nus, const uint8_t *data, uint16_t len)
{
    ARG_UNUSED(nus);
    LOG_INF("BLE Received data: %s", data);
    return BT_GATT_ITER_CONTINUE;
}

static void discovery_complete(struct bt_gatt_dm *dm, void *context)
{
    struct bt_nus_client *nus = context;
    LOG_INF("Service discovery completed");

    bt_gatt_dm_data_print(dm);

    bt_nus_handles_assign(dm, nus);
    bt_nus_subscribe_receive(nus);

    bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn, void *context)
{
    LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn, int err, void *context)
{
    LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
    .completed         = discovery_complete,
    .service_not_found = discovery_service_not_found,
    .error_found       = discovery_error,
};

void gatt_discover(struct bt_conn *conn)
{
    int err;

    if (conn != bt_uart_conn) {
        return;
    }

    err = bt_gatt_dm_start(conn, BT_UUID_NUS_SERVICE, &discovery_cb, &nus_client);
    if (err) {
        LOG_ERR("could not start the discovery procedure, error code: %d", err);
    }
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
    if (!err) {
        LOG_INF("MTU exchange done");
    } else {
        LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
    }
}

void SetupCentralConnection(struct bt_conn *conn)
{
    static struct bt_gatt_exchange_params exchange_params;

    exchange_params.func = exchange_func;
    int err = bt_gatt_exchange_mtu(conn, &exchange_params);
    if (err) {
        LOG_WRN("MTU exchange failed (err %d)", err);
    }

    err = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (err) {
        LOG_WRN("Failed to set security: %d", err);

        gatt_discover(conn);
    }

    err = bt_scan_stop();
    if ((!err) && (err != -EALREADY)) {
        LOG_ERR("Stop LE scan failed (err %d)", err);
    }
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
    char addr[BT_ADDR_LE_STR_LEN];
    int err;

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    if (conn_err) {
        LOG_INF("Failed to connect to %s (%d)", addr, conn_err);

        if (bt_uart_conn == conn) {
            bt_conn_unref(bt_uart_conn);
            bt_uart_conn = NULL;

            err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
            if (err) {
                LOG_ERR("Scanning failed to start (err %d)", err);
            }
        }

        return;
    }
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
};

static int nus_client_init(void)
{
    int err;
    struct bt_nus_client_init_param init = {
        .cb = {
            .received = ble_data_received,
            .sent = ble_data_sent,
        }
    };

    err = bt_nus_client_init(&nus_client, &init);
    if (err) {
        LOG_ERR("NUS Client initialization failed (err %d)", err);
        return err;
    }

    LOG_INF("NUS Client module initialized");
    return err;
}

void InitCentralUart(void)
{
    printk("InitCentralUart\n");
    int err;

    err = scan_init();
    if (err != 0) {
        LOG_ERR("scan_init failed (err %d)", err);
        return;
    }

    err = nus_client_init();
    if (err != 0) {
        LOG_ERR("nus_client_init failed (err %d)", err);
        return;
    }

    printk("Starting Bluetooth Central UART example\n");

    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return;
    }

    LOG_INF("Scanning successfully started\n");

    // err = bt_nus_client_send(&nus_client, buf->data, buf->len);
    // if (err) {
    //  LOG_WRN("Failed to send data over BLE connection"
    //      "(err %d)", err);
    // }
}
