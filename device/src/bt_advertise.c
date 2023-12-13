#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/services/nus.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

// HID advertisement

static const struct bt_data ad_hid[] = {
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
              (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
              (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
                      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static const struct bt_data sd_hid[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

void advertise_hid(void) {
    struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
                        BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME,
                        BT_GAP_ADV_FAST_INT_MIN_2,
                        BT_GAP_ADV_FAST_INT_MAX_2,
                        NULL);

    int err = bt_le_adv_start(adv_param, ad_hid, ARRAY_SIZE(ad_hid), sd_hid, ARRAY_SIZE(sd_hid));
    if (err) {
        if (err == -EALREADY) {
            printk("Advertising continued\n");
        } else {
            printk("Advertising failed to start (err %d)\n", err);
        }

        return;
    }

    printk("Advertising successfully started\n");
}

// Peer advertisement

static const struct bt_data ad_peer[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static const struct bt_data sd_peer[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

void advertise_peer(void) {
    int err = bt_le_adv_start(BT_LE_ADV_CONN, ad_peer, ARRAY_SIZE(ad_peer), sd_peer, ARRAY_SIZE(sd_peer));
    if (err) {
        printk("Peripheral advertising failed to start (err %d)", err);
        return 0;
    } else {
        printk("Peripheral advertising successfully started\n");
    }
}
