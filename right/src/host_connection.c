#include "host_connection.h"
#include "str_utils.h"
#include "macros/string_reader.h"

#ifdef __ZEPHYR__
#include "bt_conn.h"
#include "connections.h"

host_connection_t HostConnections[HOST_CONNECTION_COUNT_MAX] = {
    [HOST_CONNECTION_COUNT_MAX - 2] = {
        .type = HostConnectionType_NewBtHid,
        .name = (string_segment_t){ .start = "New Bluetooth Device", .end = NULL },
        .switchover = true,
    },
    [HOST_CONNECTION_COUNT_MAX - 1] = {
        .type = HostConnectionType_UsbHidRight,
        .name = (string_segment_t){ .start = "USB Device (Backup)", .end = NULL },
        .switchover = true,
    },
};

bool HostConnections_IsKnownBleAddress(const bt_addr_le_t *address) {
    for (int i = 0; i < HOST_CONNECTION_COUNT_MAX; i++) {
        switch (HostConnections[i].type) {
            case HostConnectionType_Empty:
            case HostConnectionType_UsbHidRight:
            case HostConnectionType_UsbHidLeft:
            case HostConnectionType_NewBtHid:
            case HostConnectionType_Count:
                break;
            case HostConnectionType_Dongle:
            case HostConnectionType_BtHid:
                if (BtAddrEq(address, &HostConnections[i].bleAddress)) {
                    return true;
                }
                break;
        }
    }

    // Don't count new ble connections
    // Do check devices that are paired via settings - left, right, dongle
    for (int peerIdx = 0; peerIdx < PeerIdFirstHost; peerIdx++) {
        if (BtAddrEq(address, &Peers[peerIdx].addr)) {
            return true;
        }
    }

    return false;
}

host_connection_t* HostConnection(uint8_t connectionId) {
    if (connectionId < ConnectionId_HostConnectionFirst || connectionId > ConnectionId_HostConnectionLast) {
        printk("Supplied connection (%d) doesn't correspond to a host connection!\n", connectionId);
        return NULL;
    }

    return &HostConnections[connectionId - ConnectionId_HostConnectionFirst];
}

static void selectConnection(uint8_t connectionId) {
    SelectedHostConnectionId = connectionId;
    if (Connections_IsReady(connectionId)) {
        Connections_HandleSwitchover(connectionId, true);
    } else {
        BtConn_ReserveConnections();
    }
}

static void selectNextConnection(int8_t direction) {
    for (int8_t i = ActiveHostConnectionId + direction; i != ActiveHostConnectionId; i += direction) {
        if (i > ConnectionId_HostConnectionLast) {
            i = ConnectionId_HostConnectionFirst;
        }
        if (i < ConnectionId_HostConnectionFirst) {
            i = ConnectionId_HostConnectionLast;
        }

        if (Connections_IsReady(i)) {
            Connections_HandleSwitchover(i, true);
            break;
        }
    }
    SelectedHostConnectionId = ConnectionId_Invalid;
}

void HostConnections_SelectNextConnection(void) {
    selectNextConnection(1);
}

void HostConnections_SelectPreviousConnection(void) {
    selectNextConnection(-1);
}

void HostConnections_SelectByName(parser_context_t* ctx) {
    for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
        if (Macros_CompareStringToken(ctx, HostConnection(i)->name)) {
            selectConnection(i);
            return;
        }
    }
}

void HostConnections_SelectById(uint8_t connectionId) {
    selectConnection(connectionId);
}

void HostConnections_ListKnownBleConnections() {
    printk("Known host connection ble addresses:\n");
    for (int i = 0; i < HOST_CONNECTION_COUNT_MAX; i++) {
        host_connection_type_t type = HostConnections[i].type;
        switch (type) {
            case HostConnectionType_Empty:
            case HostConnectionType_UsbHidRight:
            case HostConnectionType_UsbHidLeft:
            case HostConnectionType_Count:
                break;
            case HostConnectionType_NewBtHid:
            case HostConnectionType_Dongle:
            case HostConnectionType_BtHid:
                printk(" - %d '%.*s': address: %s\n", i, EXPAND_SEGMENT(HostConnections[i].name), GetAddrString(&HostConnections[i].bleAddress));
                break;
        }
    }
}

#endif


