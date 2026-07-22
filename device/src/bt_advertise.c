#include "bt_advertise.h"
#include <bluetooth/services/nus.h>
#include <zephyr/bluetooth/gatt.h>
#include "attributes.h"
#include "bt_conn.h"
#include "bt_pair.h"
#include "connections.h"
#include "debug.h"
#include "device.h"
#include "device_state.h"
#include "host_connection.h"
#include "keyboard/oled/widgets/widgets.h"
#include <zephyr/logging/log.h>
#include "config_manager.h"
#include "trace.h"
#include <zephyr/kernel.h>
#include "right/src/bt_defs.h"
#include "bt_health.h"

LOG_MODULE_DECLARE(Bt);

#define LEN(NAME) (sizeof(NAME) - 1)

bool BtAdvertise_IsAdvertising = false;

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

ATTR_UNUSED static const struct bt_data adNusLeft[] = {AD_NUS_DATA("UHK 80 Left BLE")};
ATTR_UNUSED static const struct bt_data adNusRight[] = {AD_NUS_DATA("UHK 80 BLE")};
static const struct bt_data adHid[] = {AD_HID_DATA};

// Helpers

#define ADVERTISEMENT(TYPE) ((adv_config_t) { .advType = TYPE })
#define ADVERTISEMENT_DIRECT_NUS(ADDR) ((adv_config_t) { .advType = ADVERTISE_DIRECTED_NUS, .addr = ADDR })

// Scan response packets

#define SD_NUS_DATA BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),

#define SD_HID_DATA(NAME) BT_DATA(BT_DATA_NAME_COMPLETE, NAME, LEN(NAME)),

static const struct bt_data sdNus[] = {SD_NUS_DATA};
static const struct bt_data sdHid[] = {SD_HID_DATA("UHK 80 BLE")};

#if DEVICE_IS_UHK80_RIGHT
#define BY_SIDE(LEFT, RIGHT) RIGHT
#else
#define BY_SIDE(LEFT, RIGHT) LEFT
#endif

// Fast advertisement makes mouse key movement jittery as it makes us miss transports
static void applyAdvInterval(struct bt_le_adv_param* param) {
    bool fast = BtConn_UnusedPeripheralConnectionCount() == ACTUAL_PERIPHERAL_CONNECTION_COUNT || Connections_IsSelectedConnecting() || !DEVICE_IS_UHK80_RIGHT;
    if (fast) {
        param->interval_min = BT_GAP_ADV_FAST_INT_MIN_1;
        param->interval_max = BT_GAP_ADV_FAST_INT_MAX_1;
    } else {
        param->interval_min = BT_GAP_ADV_SLOW_INT_MIN;
        param->interval_max = BT_GAP_ADV_SLOW_INT_MAX;
    }
}

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

ATTR_UNUSED static void setFilters(adv_config_t advConfig) {
    bt_le_filter_accept_list_clear();

    if (advConfig.advType & (ADVERTISE_HID | ADVERTISE_NUS)) {
        LOG_DBG("Bt: filling adv allow filter");
        for (uint8_t connId = ConnectionId_HostConnectionFirst; connId <= ConnectionId_HostConnectionLast; connId++) {
            host_connection_t* hostConnection = HostConnection(connId);

            if (!hostConnection) {
                continue;
            }

            if (advConfig.advType & ADVERTISE_HID && (hostConnection->type == HostConnectionType_BtHid || hostConnection->type == HostConnectionType_NewBtHid)) {
                bt_le_filter_accept_list_add(&hostConnection->bleAddress);
            }
            if (advConfig.advType & ADVERTISE_NUS && hostConnection->type == HostConnectionType_Dongle) {
                bt_le_filter_accept_list_add(&hostConnection->bleAddress);
            }
        }
    }

    if (advConfig.advType & ADVERTISE_DIRECTED_NUS) {
        LOG_DBG("Bt: filling adv allow filter for \"directed\" advertising.");
        bt_le_filter_accept_list_add(advConfig.addr);
    }
}

static void updateAdvertisingIcon(bool newAdvertising) {
    if (DEVICE_ID == DeviceId_Uhk80_Right && BtAdvertise_IsAdvertising != newAdvertising) {
        BtAdvertise_IsAdvertising = newAdvertising;
#if DEVICE_HAS_OLED
        Widget_Refresh(&StatusWidget);
#endif
    }
}

void BtAdvertise_DisableAdvertisingIcon(void) {
    updateAdvertisingIcon(false);
}

uint8_t BtAdvertise_Start(adv_config_t advConfig)
{
    BT_TRACE_AND_ASSERT("ba1");
    int err = 0;

    if ( DEVICE_IS_UHK80_RIGHT && !DeviceState_IsTargetConnected(ConnectionTarget_Left) ) {
        // In this case, scanning is in progress, so we cannot use the allow list.
        advConfig = ADVERTISEMENT(ADVERTISE_NUS | ADVERTISE_HID);
    }

    ATTR_UNUSED const char * advTypeString = advertisingString(advConfig.advType);

    // to clear filters
    BtAdvertise_Stop();

    updateAdvertisingIcon(advConfig.advType != 0);

    // Start advertising
    static struct bt_le_adv_param advParam;
    switch (advConfig.advType) {
        case ADVERTISE_HID:
        case ADVERTISE_NUS | ADVERTISE_HID:
            LOG_DBG("Adv: advertise nus+hid.");
            /* our devices don't check service uuids, so hid advertisement effectively advertises nus too */
            advParam = *BT_LE_ADV_CONN_FAST_1;
            applyAdvInterval(&advParam);
            err = BT_LE_ADV_START(&advParam, adHid, sdHid);

            break;
        case ADVERTISE_NUS:
            if (Cfg.Bt_DirectedAdvertisingAllowed) {
                LOG_DBG("Adv: advertise nus, with allow list.");
                setFilters(advConfig);
                advParam = *BT_LE_ADV_CONN_FAST_1;
                advParam.options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_USE_IDENTITY;
                applyAdvInterval(&advParam);
                err = BT_LE_ADV_START(&advParam, BY_SIDE(adNusLeft, adNusRight), sdNus);
            } else {
                LOG_DBG("Adv: advertise nus, without allow list.");
                advParam = *BT_LE_ADV_CONN_FAST_1;
                applyAdvInterval(&advParam);
                err = BT_LE_ADV_START(&advParam, BY_SIDE(adNusLeft, adNusRight), sdNus);
            }
            break;
        case ADVERTISE_DIRECTED_NUS:
            if (Cfg.Bt_DirectedAdvertisingAllowed) {
                LOG_DBG("Adv: direct advertise nus, with allow list.");
                setFilters(advConfig);

                advParam = *BT_LE_ADV_CONN_FAST_1;
                advParam.options = BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_USE_IDENTITY;

                applyAdvInterval(&advParam);
                err = BT_LE_ADV_START(&advParam, BY_SIDE(adNusLeft, adNusRight), sdNus);
            } else {
                LOG_DBG("Adv: direct advertise nus, without allow list.");
                advParam = *BT_LE_ADV_CONN_FAST_1;
                applyAdvInterval(&advParam);
                err = BT_LE_ADV_START(&advParam, BY_SIDE(adNusLeft, adNusRight), sdNus);
            }
            break;
        default:
            LOG_INF("Adv: Attempted to start advertising without any type! Ignoring.");
            return 0;
    }

    // Log it
    if (err == 0) {
        LOG_INF("Adv: '%s' started", advTypeString);
        return 0;
    } else if (err == -EALREADY) {
        LOG_INF("Adv: '%s' continued", advTypeString);
        return 0;
    } else {
        LOG_INF("Adv: '%s' failed to start (err %d), free connections: %d", advTypeString, err, BtConn_UnusedPeripheralConnectionCount());
        return err;
    }
}

void BtAdvertise_Stop(void) {
    BT_TRACE_AND_ASSERT("ba2");
    int err = bt_le_adv_stop();
    if (err) {
        LOG_WRN("Adv: Advertising failed to stop (err %d)", err);
        Bt_HandleError("BtAdvertise_Stop", err);
    }
}

adv_config_t BtAdvertise_Config() {
    switch (DEVICE_ID) {
        case DeviceId_Uhk80_Left:
            if (BtPair_OobPairingInProgress) {
                return ADVERTISEMENT(ADVERTISE_NUS | ADVERTISE_HID);
                // Fails handshake with "RF Noise?"
                // struct bt_le_oob* oob = BtPair_GetRemoteOob();
                // return ADVERTISEMENT_DIRECT_NUS(&oob->addr);
            }
            else if (Peers[PeerIdRight].conn == NULL) {
                return ADVERTISEMENT_DIRECT_NUS(&Peers[PeerIdRight].addr);
            } else {
                return ADVERTISEMENT( 0 );
            }

        case DeviceId_Uhk80_Right: {
            bool freeSlots = BtConn_UnusedPeripheralConnectionCount();
            if (freeSlots == 0) {
                LOG_INF("Adv: Current slot count is zero, not advertising!");
                return ADVERTISEMENT( 0 );
            }

            if (BtPair_OobPairingInProgress) {
                return ADVERTISEMENT(ADVERTISE_NUS | ADVERTISE_HID);
                // Fails handshake with "RF Noise?"
                // struct bt_le_oob* oob = BtPair_GetRemoteOob();
                // return ADVERTISEMENT_DIRECT_NUS(&oob->addr);
            }

            if (Connections_IsReady(CurrentHostConnectionId)) {
                return ADVERTISEMENT( 0 );
            }

            connection_type_t currentConnectionType = Connections_Type(CurrentHostConnectionId);
            if (currentConnectionType == ConnectionType_NusDongle) {
                return ADVERTISEMENT_DIRECT_NUS(&HostConnection(CurrentHostConnectionId)->bleAddress);
            } else if (currentConnectionType == ConnectionType_BtHid) {
                return ADVERTISEMENT(ADVERTISE_HID);
            } else if (currentConnectionType == ConnectionType_Empty) {
                return ADVERTISEMENT(ADVERTISE_HID);
            } else {
                return ADVERTISEMENT( 0 );
            }
        }
        case DeviceId_Uhk_Dongle:
            return ADVERTISEMENT( 0 );
        default:
            LOG_WRN("unknown device!");
            return ADVERTISEMENT( 0 );
    }
}
