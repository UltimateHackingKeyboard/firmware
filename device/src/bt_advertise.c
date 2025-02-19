#include "bt_advertise.h"
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/gatt.h>
#include "attributes.h"
#include "bt_conn.h"
#include "connections.h"
#include "device.h"

#define LEN(NAME) (sizeof(NAME) - 1)

// Advertisement packets

#define AD_NUS_DATA(NAME)                                                                                \
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),                          \
        BT_DATA(BT_DATA_NAME_COMPLETE, NAME, LEN(NAME)),

#define AD_HID_DATA                                                                                \
    BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE, (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,               \
        (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),                                                \
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),                      \
        BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),                     \
            BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),

ATTR_UNUSED static const struct bt_data adNusLeft[] = {AD_NUS_DATA("UHK80 Left Nus")};
ATTR_UNUSED static const struct bt_data adNusRight[] = {AD_NUS_DATA("UHK 80 Right")};
static const struct bt_data adHid[] = {AD_HID_DATA};

// Scan response packets

#define SD_NUS_DATA BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),

#define SD_HID_DATA(NAME) BT_DATA(BT_DATA_NAME_COMPLETE, NAME, LEN(NAME)),

static const struct bt_data sdNus[] = {SD_NUS_DATA};
static const struct bt_data sdHid[] = {SD_HID_DATA("UHK 80")};

#if DEVICE_IS_UHK80_RIGHT
#define BY_SIDE(LEFT, RIGHT) RIGHT
#else
#define BY_SIDE(LEFT, RIGHT) LEFT
#endif

#define BT_LE_ADV_START(PARAM, AD, SD) bt_le_adv_start(PARAM, AD, ARRAY_SIZE(AD), SD, ARRAY_SIZE(SD));

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
            err = BT_LE_ADV_START(&advParam, adHid, sdHid);
            break;
        case ADVERTISE_NUS:
            advParam = *BT_LE_ADV_CONN_ONE_TIME;

            err = BT_LE_ADV_START(&advParam, BY_SIDE(adNusLeft, adNusRight), sdNus);
            break;
        case ADVERTISE_DIRECTED_NUS:
            advParam = *BT_LE_ADV_CONN_ONE_TIME;
            err = BT_LE_ADV_START(&advParam, BY_SIDE(adNusLeft, adNusRight), sdNus);
            break;

            //// TODO: fix and reenable this?
            // printk("Advertising against %s\n", GetAddrString(advConfig.addr));
            // advParam = *BT_LE_ADV_CONN_DIR_LOW_DUTY(advConfig.addr);
            // advParam.options |= BT_LE_ADV_OPT_DIR_ADDR_RPA;
            // err = BT_LE_ADV_START(&advParam, BY_SIDE(adNusLeft, adNusRight), sdNus);
            break;
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
            if (Peers[PeerIdRight].conn == NULL) {
                return ADVERTISEMENT_DIRECT_NUS(&Peers[PeerIdRight].addr);
            } else {
                return ADVERTISEMENT( 0 );
            }

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

