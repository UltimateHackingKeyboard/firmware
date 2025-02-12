#include "bt_advertise.h"
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/gatt.h>
#include "bt_conn.h"
#include "connections.h"
#include "device.h"

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

static const char * advertisingString(uint8_t advType) {
    switch (advType) {
        case ADVERTISE_NUS:
            return "NUS";
        case ADVERTISE_HID:
            return "HID";
        case ADVERTISE_NUS | ADVERTISE_HID:
            return "HID \"and NUS\"";
        case ADVERTISE_DIRECTED_NUS:
            return "Directed NUS";
        default:
            return "None";
    }
}

uint8_t BtAdvertise_Start(adv_config_t advConfig)
{
    int err = 0;

    const char * advTypeString = advertisingString(advConfig.advType);

    // Start advertising
    static struct bt_le_adv_param advParam;
    switch (advConfig.advType) {
        case ADVERTISE_HID:
        case ADVERTISE_NUS | ADVERTISE_HID:
            /* our devices don't check service uuids, so hid advertisement effectively advertises nus too */
            advParam = *BT_LE_ADV_CONN_ONE_TIME;
            err = bt_le_adv_start(&advParam, adHid, ARRAY_SIZE(adHid), sdHid, ARRAY_SIZE(sdHid));
            break;
        case ADVERTISE_NUS:
            advParam = *BT_LE_ADV_CONN_ONE_TIME;
            err = bt_le_adv_start(&advParam, adNus, ARRAY_SIZE(adNus), sdNus, ARRAY_SIZE(sdNus));
            break;
        case ADVERTISE_DIRECTED_NUS:
            advParam = *BT_LE_ADV_CONN_DIR_LOW_DUTY(advConfig.addr);
            err = bt_le_adv_start(&advParam, adNus, ARRAY_SIZE(adNus), sdNus, ARRAY_SIZE(sdNus));
        default:
            printk("Adv: Attempted to start advertising without any type! Ignoring.\n");
            return 0;
    }

    // Log it
    if (err == 0) {
        printk("Adv: %s advertising successfully started\n", advTypeString);
        return 0;
    } else if (err == -EALREADY) {
        printk("Adv: %s advertising continued\n", advTypeString);
        return 0;
    } else {
        printk("Adv: %s advertising failed to start (err %d), free connections: %d\n", advTypeString, err, BtConn_UnusedPeripheralConnectionCount());
        return err;
    }
}

void BtAdvertise_Stop(void) {
    int err = bt_le_adv_stop();
    if (err) {
        printk("Adv: Advertising failed to stop (err %d)\n", err);
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

#define ADVERTISEMENT(TYPE) ((adv_config_t) { .advType = TYPE })
#define ADVERTISEMENT_DIRECT_NUS(ADDR) ((adv_config_t) { .advType = ADVERTISE_DIRECTED_NUS, .addr = ADDR })

adv_config_t BtAdvertise_Config() {
    switch (DEVICE_ID) {
        case DeviceId_Uhk80_Left:
            return ADVERTISEMENT_DIRECT_NUS(&Peers[PeerIdRight].addr);

        case DeviceId_Uhk80_Right:
            if (BtConn_UnusedPeripheralConnectionCount() > 0)  {
                if (BtConn_UnusedPeripheralConnectionCount() <= 1 && SelectedHostConnectionId != ConnectionId_Invalid) {
                    /* we need to reserve last peripheral slot for a specific target */
                    connection_type_t selectedConnectionType = Connections_Type(SelectedHostConnectionId);
                    if (selectedConnectionType == ConnectionType_NusDongle) {
                        return ADVERTISEMENT_DIRECT_NUS(&HostConnection(SelectedHostConnectionId)->bleAddress);
                    } else if (selectedConnectionType == ConnectionType_BtHid) {
                        return ADVERTISEMENT(ADVERTISE_HID);
                    } else {
                        printk("Adv: Selected connection is neither BLE HID nor NUS. Can't advertise!");
                        return ADVERTISEMENT( 0 );
                    }
                }
                else if (connectedHidCount() > 0) {
                    /** we can't handle multiple HID connections, so don't advertise it when one HID is already connected */
                    return ADVERTISEMENT(ADVERTISE_NUS);
                } else {
                    /** we can connect both NUS and HID */
                    return ADVERTISEMENT(ADVERTISE_NUS | ADVERTISE_HID);
                }
            } else {
                /** advertising needs a peripheral slot. When it is not free and we try to advertise, it will fail, and our code will try to
                 *  disconnect other devices in order to restore proper function. */
                printk("Adv: Current slot count is zero, not advertising!\n");
                BtConn_ListCurrentConnections();
                return ADVERTISEMENT( 0 );
            }
        case DeviceId_Uhk_Dongle:
            return ADVERTISEMENT( 0 );
        default:
            printk("unknown device!\n");
            return ADVERTISEMENT( 0 );
    }
}

