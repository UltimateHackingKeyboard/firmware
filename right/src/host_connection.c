#include "host_connection.h"
#include "str_utils.h"
#include "macros/string_reader.h"

#ifdef __ZEPHYR__
#include "bt_conn.h"
#include "connections.h"
#include "logger.h"
#include "keyboard/oled/widgets/widget_store.h"
#include "stubs.h"

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
        return NULL;
    }

    return &HostConnections[connectionId - ConnectionId_HostConnectionFirst];
}

void HostConnection_SetSelectedConnection(uint8_t connectionId) {
    if (SelectedHostConnectionId != connectionId) {
        SelectedHostConnectionId = connectionId;
        WIDGET_REFRESH(&TargetWidget);
    }
}

static void selectConnection(uint8_t connectionId) {
    if (Connections_IsReady(connectionId)) {
        Connections_HandleSwitchover(connectionId, true);
        HostConnection_SetSelectedConnection(ConnectionId_Invalid);
    } else {
        BtConn_ReserveConnections();
        HostConnection_SetSelectedConnection(connectionId);
    }
    Connections_ReportState(connectionId);
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

    HostConnection_SetSelectedConnection(ConnectionId_Invalid);
}

void HostConnections_SelectNextConnection(void) {
    selectNextConnection(1);
}

void HostConnections_SelectPreviousConnection(void) {
    selectNextConnection(-1);
}

void HostConnections_SelectLastConnection(void) {
    if (LastActiveHostConnectionId != ConnectionId_Invalid) {
        selectConnection(LastActiveHostConnectionId);
    }
}

void HostConnections_SelectByName(parser_context_t* ctx) {
    for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
        if (Macros_CompareStringToken(ctx, HostConnection(i)->name)) {
            selectConnection(i);
            return;
        }
    }
}

void HostConnections_SelectByHostConnIndex(uint8_t hostConnIndex) {
    uint8_t connId = ConnectionId_HostConnectionFirst + hostConnIndex;
    host_connection_t *hostConnection = HostConnection(connId);
    if (hostConnection && hostConnection->type != HostConnectionType_Empty) {
        selectConnection(connId);
    } else {
        LogUS("Invalid host connection index: %d. Ignoring!\n", hostConnIndex);
    }
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


