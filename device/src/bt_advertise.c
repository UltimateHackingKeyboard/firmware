#include "bt_advertise.h"
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/gatt.h>
#include "bt_conn.h"
#include "connections.h"
#include "device.h"
#include "event_scheduler.h"
#include "bt_scan.h"

#undef DEVICE_NAME
#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// Advertisement packets

#define AD_NUS_DATA                                                                                \
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),                          \
        BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

#define AD_HID_DATA                                                                                \
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,               \
        (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),                                                \
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),                      \
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),                     \
            BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),

static const struct bt_data adNus[] = {AD_NUS_DATA};

static const struct bt_data adHid[] = {AD_HID_DATA};

static const struct bt_data adNusHid[] = {AD_NUS_DATA AD_HID_DATA};

// Scan response packets

#define SD_NUS_DATA BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),

#define SD_HID_DATA BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

static const struct bt_data sdNus[] = {SD_NUS_DATA};

static const struct bt_data sdHid[] = {SD_HID_DATA};

static const struct bt_data sdNusHid[] = {SD_NUS_DATA SD_HID_DATA};

static struct bt_le_adv_param advertisementParams[] = BT_LE_ADV_CONN;

static void setFilters() {
    bt_le_filter_accept_list_clear();
    if (DEVICE_IS_UHK80_RIGHT) {
        if (BtConn_UnusedPeripheralConnectionCount() <= 1 && SelectedHostConnectionId != ConnectionId_Invalid) {
            bt_le_filter_accept_list_add(&HostConnection(SelectedHostConnectionId)->bleAddress);
            advertisementParams->options = BT_LE_ADV_OPT_FILTER_CONN;
        } else {
            advertisementParams->options = BT_LE_ADV_OPT_NONE;
        }
    }
}

uint8_t BtAdvertise_Start(uint8_t adv_type)
{
    setFilters();

    int err;
    const char *adv_type_string;
    if (adv_type == (ADVERTISE_NUS | ADVERTISE_HID)) {
        adv_type_string = "NUS and HID";
        err = bt_le_adv_start(
            BT_LE_ADV_CONN, adNusHid, ARRAY_SIZE(adNusHid), sdNusHid, ARRAY_SIZE(sdNusHid));
    } else if (adv_type == ADVERTISE_NUS) {
        adv_type_string = "NUS";
        err = bt_le_adv_start(BT_LE_ADV_CONN, adNus, ARRAY_SIZE(adNus), sdNus, ARRAY_SIZE(sdNus));
    } else if (adv_type == ADVERTISE_HID) {
        adv_type_string = "HID";
        err = bt_le_adv_start(BT_LE_ADV_CONN, adHid, ARRAY_SIZE(adHid), sdHid, ARRAY_SIZE(sdHid));
    } else {
        printk("Attempted to start advertising without any type! Ignoring.\n");
        return 0;
    }

    if (err == 0) {
        printk("%s advertising successfully started\n", adv_type_string);
        return 0;
    } else if (err == -EALREADY) {
        printk("%s advertising continued\n", adv_type_string);
        return 0;
    } else {
        printk("%s advertising failed to start (err %d), free connections: %d\n", adv_type_string, err, BtConn_UnusedPeripheralConnectionCount());
        return err;
    }
}

void BtAdvertise_Stop() {
    int err = bt_le_adv_stop();
    if (err) {
        printk("Advertising failed to stop (err %d)\n", err);
    }
}

static uint8_t connectedHidCount() {
    uint8_t connectedHids = 0;
    for (uint8_t peerId = PeerIdFirstHost; peerId <= PeerIdLastHost; peerId++) {
        if (Peers[peerId].conn && Connections_Type(Peers[peerId].connectionId) == ConnectionType_BtHid) {
            connectedHids++;
        }
    }
    return connectedHids;
}

uint8_t BtAdvertise_Type() {
    switch (DEVICE_ID) {
        case DeviceId_Uhk80_Left:
            return ADVERTISE_NUS;
        case DeviceId_Uhk80_Right:
            if (BtConn_UnusedPeripheralConnectionCount() > 0)  {
                if (connectedHidCount() > 0) {
                    return ADVERTISE_NUS;
                } else {
                    return ADVERTISE_NUS | ADVERTISE_HID;
                }
            } else {
                printk("Current slot count %d, not advertising\n", BtConn_UnusedPeripheralConnectionCount());
                BtConn_ListCurrentConnections();
                return 0;
            }
        case DeviceId_Uhk_Dongle:
            return 0;
        default:
            printk("unknown device!\n");
            return 0;
    }
}
