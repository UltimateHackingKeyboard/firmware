#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/nus.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// HID advertisement

static const struct bt_data adHid[] = {
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
              (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
              (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sdHid[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

void AdvertiseHid(void) {
    struct bt_le_adv_param *advParam = BT_LE_ADV_PARAM(
                        BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
                        BT_GAP_ADV_FAST_INT_MIN_2,
                        BT_GAP_ADV_FAST_INT_MAX_2,
                        NULL);

    int err = bt_le_adv_start(advParam, adHid, ARRAY_SIZE(adHid), sdHid, ARRAY_SIZE(sdHid));
    if (err) {
        if (err == -EALREADY) {
            printk("HID advertising continued\n");
        } else {
            printk("HID advertising failed to start (err %d)\n", err);
        }

        return;
    }

    printk("HID advertising successfully started\n");
}

// NUS advertisement

static const struct bt_data adNus[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sdNus[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

void AdvertiseNus(void) {
    int err = bt_le_adv_start(BT_LE_ADV_CONN, adNus, ARRAY_SIZE(adNus), sdNus, ARRAY_SIZE(sdNus));
    if (err) {
        printk("NUS advertising failed to start (err %d)", err);
    } else {
        printk("NUS advertising successfully started\n");
    }
}
