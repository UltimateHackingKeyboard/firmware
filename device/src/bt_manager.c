#include "bt_manager.h"
#include "bt_pair.h"
#include "connections.h"
#include "device_state.h"
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
#include "config_manager.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(Bt);

#define BT_SHORT_RETRY_DELAY 1000

bool BtManager_Restarting = false;

static void bt_ready(int err)
{
    if (err) {
        LOG_WRN("Bluetooth init failed (err %d)\n", err);
    } else {
        LOG_INF("Bluetooth initialized successfully\n");
    }
}

void BtManager_InitBt() {
    BtConn_Init();

    if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
        int err = NusServer_Init();
        if (err) {
            LOG_WRN("NusServer_Init failed with error %d\n", err);
        }
    }

    if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
        BtScan_Init();
        NusClient_Init();
    }
}

void BtManager_StartBt() {
    LOG_INF("Starting bluetooth services.\n");

    if (!Cfg.Bt_Enabled) {
        return;
    }

    if (DEVICE_IS_UHK80_RIGHT) {
        HOGP_Enable();
    }

    BtManager_StartScanningAndAdvertising();
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


void BtManager_StartScanningAndAdvertisingAsync() {
    uint32_t delay = 50;
    EventScheduler_Reschedule(CurrentTime + delay, EventSchedulerEvent_BtStartScanningAndAdvertising, "BtManager_StartScanningAndAdvertising");
}

/*
 * Retry logic:
 * - first try: start advertising and scanning
 * - second try: stop and then start advertising and scanning
 * - third try: disconnect all connections, stop and then start advertising and scanning
 *
 * This should be called from the event scheduler.
 */
void BtManager_StartScanningAndAdvertising() {
    bool success = true;
    static uint8_t try = 0;
    int err = 0;

    if (!Cfg.Bt_Enabled) {
        return;
    }

    if (try > 0) {
        if (DEVICE_IS_UHK80_LEFT || DEVICE_IS_UHK80_RIGHT) {
            BtAdvertise_Stop();
        }

        if (DEVICE_IS_UHK80_RIGHT || DEVICE_IS_UHK_DONGLE) {
            BtScan_Stop();
        }
    }

    bool leftShouldAdvertise = DEVICE_IS_UHK80_LEFT && Peers[PeerIdRight].conn == NULL;
    bool rightShouldAdvertise = DEVICE_IS_UHK80_RIGHT && true;
    bool shouldAdvertise = leftShouldAdvertise || rightShouldAdvertise;

    bool rightShouldScanForPeer = DEVICE_IS_UHK80_RIGHT && BtPair_PairingMode != PairingMode_Oob && !DeviceState_IsTargetConnected(ConnectionTarget_Left);
    bool rightShouldScanForOob = DEVICE_IS_UHK80_RIGHT && BtPair_PairingMode == PairingMode_Oob && BtPair_PairingAsCentral;
    bool dongleShouldScanForPeer = DEVICE_IS_UHK_DONGLE && BtPair_PairingMode != PairingMode_Oob && Peers[PeerIdRight].conn == NULL;
    bool dongleShouldScanForOob = DEVICE_IS_UHK_DONGLE && BtPair_PairingMode == PairingMode_Oob && BtPair_PairingAsCentral;
    bool shouldScan = rightShouldScanForPeer || rightShouldScanForOob || dongleShouldScanForPeer || dongleShouldScanForOob;

    printk("==== btManager: should scanAndAdvertise %d %d\n", shouldScan, shouldAdvertise);

    if (shouldAdvertise || shouldScan) {
        const char* label = "";
        if (shouldAdvertise && shouldScan) {
            label = "advertising and scanning";
        } else if (shouldAdvertise) {
            label = "advertising";
        } else if (shouldScan) {
            label = "scanning";
        }
        LOG_INF("Starting %s, try %d!\n", label, try);
    }

#ifdef CONFIG_BT_PERIPHERAL
    if (!shouldScan && shouldAdvertise) {
        err = BtAdvertise_Start(BtAdvertise_Config());
        success &= err == 0;
    }
#endif

#ifdef CONFIG_BT_CENTRAL
    if (shouldScan) {
        err = BtScan_Start();
        success &= err == 0;
    }
#endif


    if (!success && try > 0) {
        BtConn_DisconnectAll();
    }

    if (success) {
        try = 0;
    } else {
        try++;
        uint32_t delay = try < 3 ? BT_SHORT_RETRY_DELAY : 10000;
        EventScheduler_Reschedule(CurrentTime + delay, EventSchedulerEvent_BtStartScanningAndAdvertising, "BtManager_StartScanningAndAdvertising failed");
    }
}

void BtManager_RestartBt() {
    LOG_INF("Going to reset bluetooth stack\n");

    BtManager_Restarting = true;
    int err;

    BtManager_StopBt();

    BtConn_DisconnectAll();

    err = bt_disable();
    if (err) {
        LOG_WRN("Bluetooth disable failed (err %d)\n", err);
        return;
    }

    //wait for them to properly disconnect
    k_sleep(K_MSEC(100));

    err = bt_hci_cmd_send(BT_HCI_OP_RESET, NULL);
    if (err) {
        LOG_WRN("HCI Reset failed (err %d)\n", err);
    }

    err = bt_enable(bt_ready);
    if (err) {
        LOG_WRN("Bluetooth init failed (err %d)\n", err);
    }

    Settings_Reload();

    BtManager_StartBt();

    BtManager_Restarting = false;

    LOG_INF("Bluetooth subsystem restart finished\n");
}

void BtManager_RestartBtAsync() {
    EventScheduler_Schedule(k_uptime_get()+BT_SHORT_RETRY_DELAY, EventSchedulerEvent_RestartBt, "Restart bluetooth");
}
