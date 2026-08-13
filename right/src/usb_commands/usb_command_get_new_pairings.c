#include "usb_command_get_new_pairings.h"
#include "usb_protocol_handler.h"
#include "bt_conn.h"
#include <zephyr/bluetooth/addr.h>
#include "connections.h"
#include "host_connection.h"

#define ADDRESS_COUNT_PER_PAGE 10
#define ADDRESS_AND_SLOT_COUNT_PER_PAGE 8

#define HEADER_LENGTH 2

typedef struct {
    const uint8_t *OutBuffer;
    uint8_t *InBuffer;
    uint8_t pageIdxOffset;
    uint8_t writeOffset;
    uint8_t addressCount;
    bool withSlots;
    bool dryRun;
} CommandUserData;

static uint8_t entryLength(bool withSlots) {
    return withSlots ? BLE_ADDR_LEN + 1 : BLE_ADDR_LEN;
}

static uint8_t entriesPerPage(bool withSlots) {
    return withSlots ? ADDRESS_AND_SLOT_COUNT_PER_PAGE : ADDRESS_COUNT_PER_PAGE;
}

static void bt_foreach_bond_cb(const struct bt_bond_info *info, void *user_data)
{
    CommandUserData *data = (CommandUserData *)user_data;
    uint8_t *GenericHidInBuffer = data->InBuffer;

    uint8_t connectionId;

    if (HostConnections_LookupBleAddress(&info->addr, &connectionId) != HostKnown_Unregistered) {
        return;
    }

    Bt_NewPairedDevice = true;

    uint8_t addressIdx = data->addressCount++;

    if (data->dryRun) {
        return;
    }

    if (addressIdx < data->pageIdxOffset || addressIdx >= data->pageIdxOffset + entriesPerPage(data->withSlots)) {
        return;
    }

    if (data->writeOffset + entryLength(data->withSlots) > USB_COMMAND_BUFFER_LENGTH) {
        return;
    }

    SetUsbTxBufferBleAddress(data->writeOffset, &info->addr);
    data->writeOffset += BLE_ADDR_LEN;

    if (data->withSlots) {
        SetUsbTxBufferUint8(data->writeOffset, connectionId - ConnectionId_HostConnectionFirst);
        data->writeOffset += 1;
    }
}

void UsbCommand_UpdateNewPairingsFlag() {
    Bt_NewPairedDevice = false;

    CommandUserData data = {
        .OutBuffer = NULL,
        .InBuffer = NULL,
        .pageIdxOffset = 0,
        .writeOffset = HEADER_LENGTH,
        .addressCount = 0,
        .withSlots = false,
        .dryRun = true,
    };

    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_bond_cb, &data);
}

void UsbCommand_GetNewPairings(uint8_t page, bool withSlots, const uint8_t *GenericHidOutBuffer, uint8_t *GenericHidInBuffer) {
    CommandUserData data = {
        .OutBuffer = GenericHidOutBuffer,
        .InBuffer = GenericHidInBuffer,
        .pageIdxOffset = entriesPerPage(withSlots)*page,
        .writeOffset = HEADER_LENGTH,
        .addressCount = 0,
        .withSlots = withSlots,
        .dryRun = false,
    };

    bt_foreach_bond(BT_ID_DEFAULT, bt_foreach_bond_cb, &data);

    if (data.addressCount < data.pageIdxOffset) {
        SetUsbTxBufferUint8(1, 0);
    } else {
        SetUsbTxBufferUint8(1, data.addressCount-data.pageIdxOffset);
    }
}
