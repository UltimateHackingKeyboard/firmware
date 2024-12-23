#include <bluetooth/scan.h>
#include "bt_conn.h"
#include "bt_pair.h"
#include "device.h"
#include "bt_pair.h"
#include "event_scheduler.h"
#include "host_connection.h"

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

int BtScan_Stop(void) {
    int err;

    bt_scan_stop();
    bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    err = bt_scan_stop();
    if (err) {
        printk("Stop LE scan failed (err %d)\n", err);
    } else {
        printk("Scanning successfully stopped.\n");
    }

    bt_scan_filter_disable();

    bt_scan_filter_remove_all();

    return err;
}

int BtScan_Init(void) {
    struct bt_scan_init_param scan_init = {
        .connect_if_match = 1,
    };

    bt_scan_init(&scan_init);
    bt_scan_cb_register(&scan_cb);

    printk("Scan module initialized\n");
    return 0;
}

int BtScan_Start(void) {
    int err;

    scan_fill();

    err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
    if (err) {
        printk("Filters cannot be turned on (err %d)\n", err);
        return err;
    }

    err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
    if (err) {
        printk("Scanning failed to start (err %d)\n", err);
        EventScheduler_Schedule(CurrentTime + 1000, EventSchedulerEvent_BtStartScanning, "BtStartScanning");
    } else {
        printk("Scanning successfully started\n");
    }

    return err;
}
