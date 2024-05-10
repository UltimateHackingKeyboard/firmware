#include <bluetooth/scan.h>
#include "bt_conn.h"
#include "device.h"

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

int scan_init(void) {
    struct bt_scan_init_param scan_init = {
        .connect_if_match = 1,
    };

    bt_scan_init(&scan_init);
    bt_scan_cb_register(&scan_cb);

    int err;
    if (DEVICE_IS_UHK80_RIGHT) {
        err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, &Peers[PeerIdLeft].addr);
        if (err) {
            printk("Scanning filters cannot be set (err %d)\n", err);
            return err;
        }
    }
    if (DEVICE_IS_UHK_DONGLE) {
        err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, &Peers[PeerIdRight].addr);
        if (err) {
            printk("Scanning filters cannot be set (err %d)\n", err);
            return err;
        }
    }

    err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
    if (err) {
        printk("Filters cannot be turned on (err %d)\n", err);
        return err;
    }

    printk("Scan module initialized\n");
    return err;
}
