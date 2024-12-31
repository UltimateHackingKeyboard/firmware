#include "bt_manager.h"
#include "event_scheduler.h"
#include "usb/usb.h"
#include "bt_advertise.h"
#include "nus_client.h"
#include "nus_server.h"
#include "event_scheduler.h"
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/settings/settings.h>
#include "bt_conn.h"
#include "bt_scan.h"
#include "settings.h"

bool BtManager_Restarting = false;

static void bt_ready(int err)
{
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
    } else {
        printk("Bluetooth initialized successfully\n");
    }
}

void BtManager_InitBt() {
    BtConn_Init();

    if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
        int err = NusServer_Init();
        if (err) {
            printk("NusServer_Init failed with error %d\n", err);
        }
    }

    if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
        BtScan_Init();
        NusClient_Init();
    }
}

void BtManager_StartBt() {
    printk("Starting bluetooth services.\n");

    if (DEVICE_IS_UHK80_RIGHT) {
        HOGP_Enable();
    }

    if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
        BtAdvertise_Start(BtAdvertise_Type());
    }

    if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
        // This scan effectively initiates NUS client connection.
        BtScan_Start();
    }
}

void BtManager_StopBt() {
    if (DEVICE_IS_UHK80_RIGHT) {
        HOGP_Disable();
    }

    if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
        BtAdvertise_Stop();
    }

    if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
        BtScan_Stop();
    }
}

void BtManager_StartScanningAndAdvertising() {
    if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
        BtAdvertise_Stop();
    }

    if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
        BtScan_Stop();
    }

    bool leftShouldAdvertise = DEVICE_IS_UHK80_LEFT && Peers[PeerIdRight].conn == NULL;
    bool rightShouldAdvertise = DEVICE_IS_UHK80_RIGHT && true;
    if (leftShouldAdvertise || rightShouldAdvertise) {
        BtAdvertise_Start(BtAdvertise_Type());
    }

    bool rightShouldScan = DEVICE_IS_UHK80_RIGHT && Peers[PeerIdLeft].conn == NULL;
    bool dongleShouldScan = DEVICE_IS_UHK_DONGLE && Peers[PeerIdRight].conn == NULL;
    if (rightShouldScan || dongleShouldScan) {
        BtScan_Start();
    }
}

void BtManager_RestartBt() {
    printk("Going to reset bluetooth stack\n");

    BtManager_Restarting = true;
    int err;

    BtManager_StopBt();

    BtConn_DisconnectAll();

    err = bt_disable();
    if (err) {
        printk("Bluetooth disable failed (err %d)\n", err);
        return;
    }

    //wait for them to properly disconnect
    k_sleep(K_MSEC(100));

    err = bt_hci_cmd_send(BT_HCI_OP_RESET, NULL);
    if (err) {
        printk("HCI Reset failed (err %d)\n", err);
    }

    err = bt_enable(bt_ready);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
    }

    Settings_Reload();

    BtManager_StartBt();

    BtManager_Restarting = false;

    printk("Bluetooth subsystem restart finished\n");
}

void BtManager_RestartBtAsync() {
    EventScheduler_Schedule(k_uptime_get()+10, EventSchedulerEvent_RestartBt, "Restart bluetooth");
}
