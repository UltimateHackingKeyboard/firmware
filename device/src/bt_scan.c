#include <bluetooth/scan.h>
#include "bt_conn.h"
#include "bt_pair.h"
#include "device.h"
#include "legacy/host_connection.h"

static void scan_filter_match(struct bt_scan_device_info *device_info,
    struct bt_scan_filter_match *filter_match, bool connectable)
{
    printk("Filters matched: %s, connectable:%d\n",
        GetPeerStringByAddr(device_info->recv_info->addr), connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info) {
    printk("Connecting failed: %s\n", GetPeerStringByAddr(device_info->recv_info->addr));
}

static void scan_connecting(struct bt_scan_device_info *device_info, struct bt_conn *conn) {
    printk("Scan connecting: %s\n", GetPeerStringByAddr(device_info->recv_info->addr));
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL, scan_connecting_error, scan_connecting);

static void addAddress(int* err, bt_addr_le_t* addr) {
    if (*err) {
        return;
    }

    *err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, addr);
}

static int scan_fill() {
    int err = 0;

    if (DEVICE_IS_UHK80_RIGHT) {
        if (!BtPair_OobPairingInProgress) {
            // iterate host connections and add all that are of type dongle
            for (uint8_t i = 0; i < HOST_CONNECTION_COUNT_MAX; i++) {
                if (HostConnections[i].type == HostConnectionType_Dongle) {
                    addAddress(&err, &HostConnections[i].bleAddress);
                }
            }
        }

        addAddress(&err, &Peers[PeerIdLeft].addr);
    }
    if (DEVICE_IS_UHK_DONGLE) {
        addAddress(&err, &Peers[PeerIdRight].addr);
    }

    if (err) {
        printk("Scanning filters cannot be set (err %d)\n", err);
        return err;
    }
    return err;
}

int scan_reload_filters(void) {
    int err;

    bt_scan_stop();

    bt_scan_filter_disable();

    bt_scan_filter_remove_all();

    scan_fill();

    err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
    if (err) {
        printk("Filters cannot be turned on (err %d)\n", err);
        return err;
    }

    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        printk("Scanning failed to start (err %d)\n", err);
        return err;
    }

    printk("reloaded scan filters\n");
    return err;
}

int scan_init(void) {
    int err;
    struct bt_scan_init_param scan_init = {
        .connect_if_match = 1,
    };

    bt_scan_init(&scan_init);
    bt_scan_cb_register(&scan_cb);

    scan_fill();

    err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
    if (err) {
        printk("Filters cannot be turned on (err %d)\n", err);
        return err;
    }

    printk("Scan module initialized\n");
    return err;
}
