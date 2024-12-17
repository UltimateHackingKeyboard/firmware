#include "usb_command_get_new_pairings.h"
#include "usb_protocol_handler.h"

#ifdef __ZEPHYR__

#include "bt_conn.h"
#include <zephyr/bluetooth/addr.h>
#include "host_connection.h"

typedef struct {
    const uint8_t *OutBuffer;
    uint8_t *InBuffer;
    uint8_t idx;
    uint8_t count;
} CommandUserData;


static void bt_foreach_bond_cb(const struct bt_bond_info *info, void *user_data)
{
    CommandUserData *data = (CommandUserData *)user_data;
    uint8_t *GenericHidInBuffer = data->InBuffer;

    if ((data->idx + BLE_ADDR_LEN + 1) >= USB_GENERIC_HID_IN_BUFFER_LENGTH) {
        return;
    }

    if (HostConnections_IsKnownBleAddress(&info->addr)) {
        return;
    }

    data->count++;

    SetUsbTxBufferBleAddress(data->idx, &info->addr);
    data->idx += BLE_ADDR_LEN;

    // Name placeholder
    SetUsbTxBufferUint8(data->idx++, 0);
}

void UsbCommand_GetNewPairings(const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer) {
    CommandUserData data = {
        .OutBuffer = GenericHidOutBuffer,
        .InBuffer = GenericHidInBuffer,
        .idx = 2,
        .count = 0
    };

    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_bond_cb, &data);

    SetUsbTxBufferUint8(1, data.count);

    Bt_NewPairedDevice = false;
}

#endif

