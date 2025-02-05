#include "bt_advertise.h"
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/gatt.h>
#include "bt_conn.h"
#include "connections.h"
#include "device.h"
#include "event_scheduler.h"
#include "bt_scan.h"

#define ADV_DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define ADV_DEVICE_NAME_LEN (sizeof(CONFIG_BT_DEVICE_NAME) - 1)

// Advertisement packets

#define AD_NUS_DATA                                                                                \
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),                          \
        BT_DATA(BT_DATA_NAME_COMPLETE, ADV_DEVICE_NAME, ADV_DEVICE_NAME_LEN),

#define AD_HID_DATA                                                                                \
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,               \
        (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),                                                \
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),                      \
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),                     \
            BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),

static const struct bt_data adNus[] = {AD_NUS_DATA};
static const struct bt_data adHid[] = {AD_HID_DATA};

// Scan response packets

#define SD_NUS_DATA BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),

#define SD_HID_DATA BT_DATA(BT_DATA_NAME_COMPLETE, ADV_DEVICE_NAME, ADV_DEVICE_NAME_LEN),

static const struct bt_data sdNus[] = {SD_NUS_DATA};
static const struct bt_data sdHid[] = {SD_HID_DATA};

int BtAdvertise_Start(void)
{
    int err = 0;
    // directed advertising
    static struct bt_le_adv_param adv_param;
    if (DEVICE_IS_UHK80_RIGHT) {
        adv_param = *BT_LE_ADV_CONN_ONE_TIME;

        err = bt_le_adv_start(&adv_param, adHid, ARRAY_SIZE(adHid), sdHid, ARRAY_SIZE(sdHid));

    } else if (DEVICE_IS_UHK80_LEFT) {
        adv_param = *BT_LE_ADV_CONN_DIR_LOW_DUTY(&HostConnection(ConnectionId_NusClientRight)->bleAddress);
        // adv_param.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;
        err = bt_le_adv_start(&adv_param, adNus, ARRAY_SIZE(adNus), sdNus, ARRAY_SIZE(sdNus));

    } else {
        return 0;
    }

    if (err == 0) {
        printk("Advertising successfully started\n");
        return 0;
    } else if (err == -EALREADY) {
        printk("Advertising continued\n");
        return 0;
    } else {
        printk("Advertising failed to start (err %d), free connections: %d\n", err, BtConn_UnusedPeripheralConnectionCount());
        return err;
    }
}

void BtAdvertise_Stop(void) {
    int err = bt_le_adv_stop();
    if (err) {
        printk("Advertising failed to stop (err %d)\n", err);
    }
}
