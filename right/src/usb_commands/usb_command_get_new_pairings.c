#include "usb_command_get_new_pairings.h"
#include "usb_protocol_handler.h"

#ifdef __ZEPHYR__

#include "bt_conn.h"
#include <zephyr/bluetooth/addr.h>
#include "host_connection.h"

#define ADDRESS_COUNT_PER_PAGE 10

static uint8_t pageIdxOffset;
static uint8_t writeOffset;
static uint8_t count;

static bool dryRun = false;

static void bt_foreach_bond_cb(const struct bt_bond_info *info, void *user_data)
{
    if (writeOffset + BLE_ADDR_LEN+1 >= USB_GENERIC_HID_IN_BUFFER_LENGTH) {
        return;
    }

    if (HostConnections_IsKnownBleAddress(&info->addr)) {
        return;
    }

    Bt_NewPairedDevice = true;

    count++;

    if (!dryRun && count >= pageIdxOffset && count < pageIdxOffset+ADDRESS_COUNT_PER_PAGE) {
        SetUsbTxBufferBleAddress(writeOffset, &info->addr);
        writeOffset += BLE_ADDR_LEN;
    }

}

void UsbCommand_UpdateNewPairingsFlag() {
    dryRun = true;
    pageIdxOffset = 0;
    count = 0;

    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_bond_cb, NULL);
}

void UsbCommand_GetNewPairings(uint8_t page) {
    dryRun = false;
    pageIdxOffset = ADDRESS_COUNT_PER_PAGE*page;
    count = 0;
    writeOffset = 2;

    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_bond_cb, NULL);

    if (count < pageIdxOffset) {
        SetUsbTxBufferUint8(1, 0);
    } else {
        SetUsbTxBufferUint8(1, count-pageIdxOffset);
    }
}

#endif

