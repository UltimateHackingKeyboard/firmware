#include <bluetooth/services/nus.h>
#include <bluetooth/services/nus_client.h>
#include <bluetooth/scan.h>
#include <zephyr/logging/log.h>
#include "bt_scan.h"
#include "bt_conn.h"
#include "bt_central_uart.h"
#include "bool_array_converter.h"
#include "legacy/slot.h"
#include "shared/bool_array_converter.h"
#include "legacy/module.h"
#include "legacy/key_states.h"
#include "keyboard/oled/widgets/console_widget.h"

#define LOG_MODULE_NAME central_uart
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static struct bt_nus_client nus_client;

static void ble_data_sent(struct bt_nus_client *nus, uint8_t err, const uint8_t *const data, uint16_t len)
{
    printk("NUS data sent to %s: %i\n", GetPeerStringByConn(nus->conn), len);
    if (err) {
        LOG_WRN("ATT error code: 0x%02X", err);
    }
}

static uint8_t ble_data_received(struct bt_nus_client *nus, const uint8_t *data, uint16_t len)
{
#if DEVICE_IS_UHK80_RIGHT
    printk("Received data: %i %i %i %i %i %i %i %i\n", data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
    for (uint8_t keyId = 0; keyId < MAX_KEY_COUNT_PER_MODULE; keyId++) {
        KeyStates[SlotId_LeftKeyboardHalf][keyId].hardwareSwitchState = !!(data[keyId/8] & (1 << (keyId % 8)));
    }
#endif
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
    int err = bt_gatt_dm_start(conn, BT_UUID_NUS_SERVICE, &discovery_cb, &nus_client);
    if (err) {
        LOG_ERR("could not start the discovery procedure, error code: %d", err);
    }
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
    if (!err) {
        LOG_INF("MTU exchange done with %s", GetPeerStringByConn(conn));
    } else {
        LOG_WRN("MTU exchange failed with %s (err %u)", GetPeerStringByConn(conn), err);
    }
}

void SetupCentralConnection(struct bt_conn *conn)
{
    static struct bt_gatt_exchange_params exchange_params;

    exchange_params.func = exchange_func;
    int err = bt_gatt_exchange_mtu(conn, &exchange_params);
    if (err) {
        LOG_WRN("MTU exchange failed with %s, err %d", GetPeerStringByConn(conn), err);
    }

    err = bt_conn_set_security(conn, BT_SECURITY_L2);
    if (err) {
        LOG_WRN("Failed to set security for %s: %d", GetPeerStringByConn(conn), err);
    }

    gatt_discover(conn);

    err = bt_scan_stop();
    if ((!err) && (err != -EALREADY)) {
        LOG_ERR("Stop LE scan failed (err %d)", err);
    }
}

static int nus_client_init(void)
{
    struct bt_nus_client_init_param init = {
        .cb = {
            .received = ble_data_received,
            .sent = ble_data_sent,
        }
    };

    int err = bt_nus_client_init(&nus_client, &init);
    if (err) {
        LOG_ERR("NUS Client initialization failed (err %d)", err);
        return err;
    }

    LOG_INF("NUS Client module initialized");
    return err;
}

void InitCentralUart(void)
{
    int err = scan_init();
    if (err != 0) {
        LOG_ERR("scan_init failed (err %d)", err);
        return;
    }

    err = nus_client_init();
    if (err != 0) {
        LOG_ERR("nus_client_init failed (err %d)", err);
        return;
    }

    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        LOG_ERR("Scanning failed to start (err %d)", err);
        return;
    }

    LOG_INF("Scanning successfully started");
}

void SendCentralUart(const uint8_t *data, uint16_t len)
{
    int err = bt_nus_client_send(&nus_client, data, len);
    if (err) {
        LOG_WRN("Failed to send data over BLE connection (err %d)", err);
    }
}
