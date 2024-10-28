#include "usb_command_get_new_pairings.h"
#include "usb_protocol_handler.h"

#ifdef __ZEPHYR__

#include "bt_conn.h"
#include <zephyr/bluetooth/addr.h>
#include "host_connection.h"

static uint8_t idx;
static uint8_t count;

static void bt_foreach_bond_cb(const struct bt_bond_info *info, void *user_data)
{
    if (idx + BLE_ADDR_LEN+1 >= USB_GENERIC_HID_IN_BUFFER_LENGTH) {
        return;
    }

    if (HostConnections_IsKnownBleAddress(&info->addr)) {
        return;
    }

    count++;

    SetUsbTxBufferBleAddress(idx, &info->addr);
    idx += BLE_ADDR_LEN;

    // Name placeholder
    SetUsbTxBufferUint8(idx++, 0);
}

void UsbCommand_GetNewPairings(void) {
    count = 0;
    idx = 2;

    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_bond_cb, NULL);

    SetUsbTxBufferUint8(1, count);

    Bt_NewPairedDevice = false;
}

#endif

