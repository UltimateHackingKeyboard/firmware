#include "host_connection.h"

#ifdef __ZEPHYR__
#include "bt_conn.h"
#include "connections.h"

host_connection_t HostConnections[HOST_CONNECTION_COUNT_MAX] = {};

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
                if (bt_addr_le_cmp(address, &HostConnections[i].bleAddress) == 0) {
                    return true;
                }
                break;
        }
    }

    for (int peerIdx = 0; peerIdx < PeerCount; peerIdx++) {
        if (bt_addr_le_cmp(address, &Peers[peerIdx].addr) == 0) {
            return true;
        }
    }

    return false;
}

host_connection_t* HostConnection(connection_id_t connectionId) {
    if (connectionId < ConnectionId_HostConnectionFirst || connectionId > ConnectionId_HostConnectionLast) {
        printk("Supplied connection doesn't correspond to a host connection!\n");
        return NULL;
    }

    return &HostConnections[connectionId - ConnectionId_HostConnectionFirst];
}

static void selectConnection(int8_t direction) {
    for (uint8_t i = TargetConnectionId + direction; i != TargetConnectionId; ) {
        if (Connections_IsReady(i)) {
            Connections_HandleSwitchover(i, true);
            return;
        }

        i+= direction;
        if (i > ConnectionId_HostConnectionLast) {
            i = ConnectionId_HostConnectionFirst;
        }
        if (i < ConnectionId_HostConnectionFirst) {
            i = ConnectionId_HostConnectionLast;
        }
    }
}

void HostConnections_SelectNextConnection(void) {
    selectConnection(1);
}

void HostConnections_SelectPreviousConnection(void) {
    selectConnection(-1);
}

void HostConnections_SelectByName(string_segment_t name) {
    for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
        if (Connections_IsReady(i) && SegmentEqual(name, HostConnection(i)->name)) {
            Connections_HandleSwitchover(i, true);
            return;
        }
    }
}

#endif


