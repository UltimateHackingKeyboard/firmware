#include "bt_advertise.h"
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/gatt.h>

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

void Advertise(uint8_t adv_type)
{
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
        assert(false);
        return;
    }

    if (err == 0) {
        printk("%s advertising successfully started\n", adv_type_string);
    } else if (err == -EALREADY) {
        printk("%s advertising continued\n", adv_type_string);
    } else {
        printk("%s advertising failed to start (err %d)\n", adv_type_string, err);
    }
}
