#include "bt_health.h"
#include "bt_conn.h"
#include "logger.h"
#include "trace.h"
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gap.h>
#include "bt_advertise.h"
#include "connections.h"
#include "hid/transport.h"

LOG_MODULE_DECLARE(Bt);

#define MAX_BONDS_ALLOWED 8 // Adjust as appropriate for your device

static void bond_count_cb(const struct bt_bond_info *info, void *user_data) {
    (*(int*)user_data)++;
}

void Bt_HealthCheck(log_target_t targetOnError, bool isError) {
    BT_TRACE_AND_ASSERT("bh1");

    log_target_t target = LogTarget_Uart | (isError ? targetOnError : 0);


    // 1. Check all Peers for valid pointers and state mismatches
    for (uint8_t i = PeerIdFirstHost; i <= PeerIdLastHost; i++) {
        struct bt_conn* conn = Peers[i].conn;
        if (conn) {
            struct bt_conn_info info;
            int err = bt_conn_get_info(conn, &info);
            if (err) {
                LogTo(DEVICE_ID, target, "HealthCheck: Peer %d has invalid conn pointer!", i);
            }
        }
    }

    // 2. HOGP health check
    if (DEVICE_IS_UHK80_RIGHT) {
        HOGP_HealthCheck();
    }

    // 3. Check bond count
    int bond_count = 0;
    bt_foreach_bond(BT_ID_DEFAULT, bond_count_cb, &bond_count);
    if (bond_count > MAX_BONDS_ALLOWED) {
        LogTo(DEVICE_ID, target, "HealthCheck: Bond count (%d) exceeds allowed maximum (%d)!", bond_count, MAX_BONDS_ALLOWED);
    }

    // 4. (Resource leak check placeholder)
    // Zephyr limits connection count, but you could check for unexpected open connections here.

    // 5. (Controller liveness check placeholder)
    // Could add a heartbeat or HCI command check in the future.

    // 6. Print trace buffer if reason is serious
    if (isError) {
        Trace_Print(target, "Bluetooth health check");
        Connections_PrintInfo(target);
    }
}

void Bt_HandleError(const char* context, int err) {
    log_target_t target = LogTarget_ErrorBuffer | LogTarget_Uart;
    LogTo(DEVICE_ID, target, "Bluetooth error in %s: %d", context, err);
    Bt_HealthCheck(target, true);

    // Optionally: take recovery action, e.g., restart BT stack
    // BtManager_RestartBt();
}
