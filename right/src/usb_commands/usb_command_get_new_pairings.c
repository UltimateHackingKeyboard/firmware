#include "usb_command_get_new_pairings.h"
#include "usb_protocol_handler.h"

#ifdef __ZEPHYR__

#include "bt_conn.h"
#include <zephyr/bluetooth/addr.h>
#include "host_connection.h"

#define ADDRESS_COUNT_PER_PAGE 10

typedef struct {
    const uint8_t *OutBuffer;
    uint8_t *InBuffer;
    uint8_t pageIdxOffset;
    uint8_t writeOffset;
    uint8_t addressCount;
    bool dryRun;
} CommandUserData;

static void bt_foreach_bond_cb(const struct bt_bond_info *info, void *user_data)
{
    CommandUserData *data = (CommandUserData *)user_data;
    uint8_t *GenericHidInBuffer = data->InBuffer;

    if ((data->writeOffset + BLE_ADDR_LEN + 1) >= USB_GENERIC_HID_IN_BUFFER_LENGTH) {
        return;
    }

    if (HostConnections_IsKnownBleAddress(&info->addr)) {
        return;
    }

    Bt_NewPairedDevice = true;

    data->addressCount++;

    if (!data->dryRun && data->addressCount >= data->pageIdxOffset && data->addressCount < data->pageIdxOffset+ADDRESS_COUNT_PER_PAGE) {
        SetUsbTxBufferBleAddress(data->writeOffset, &info->addr);
        data->writeOffset += BLE_ADDR_LEN;
    }

}

void UsbCommand_UpdateNewPairingsFlag() {

    CommandUserData data = {
        .OutBuffer = NULL,
        .InBuffer = NULL,
        .pageIdxOffset = 0,
        .writeOffset = 2,
        .addressCount = 0,
        .dryRun = true,
    };

    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_bond_cb, &data);
}

void UsbCommand_GetNewPairings(uint8_t page, const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer) {
    CommandUserData data = {
        .OutBuffer = GenericHidOutBuffer,
        .InBuffer = GenericHidInBuffer,
        .pageIdxOffset = ADDRESS_COUNT_PER_PAGE*page,
        .writeOffset = 2,
        .addressCount = 0,
        .dryRun = false,
    };

    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_bond_cb, &data);

    if (data.addressCount < data.pageIdxOffset) {
        SetUsbTxBufferUint8(1, 0);
    } else {
        SetUsbTxBufferUint8(1, data.addressCount-data.pageIdxOffset);
    }
}

#endif

