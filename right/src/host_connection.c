#include "host_connection.h"
#include "macros/status_buffer.h"
#include "str_utils.h"
#include "macros/string_reader.h"

#ifdef __ZEPHYR__
#include "bt_conn.h"
#include "connections.h"
#include "logger.h"
#include "keyboard/oled/widgets/widget_store.h"
#include "stubs.h"
#include "debug.h"
#include "bt_pair.h"

host_connection_t HostConnections[HOST_CONNECTION_COUNT_MAX] = {
    [HOST_CONNECTION_COUNT_MAX - 2] = {
        .type = HostConnectionType_NewBtHid,
        .name = (string_segment_t){ .start = "Unregistered Ble", .end = NULL },
        .switchover = true,
    },
    [HOST_CONNECTION_COUNT_MAX - 1] = {
        .type = HostConnectionType_UsbHidRight,
        .name = (string_segment_t){ .start = "USB Device (Backup)", .end = NULL },
        .switchover = true,
    },
};

host_known_t HostConnections_IsKnownBleAddress(const bt_addr_le_t *address) {
    for (int i = 0; i < HOST_CONNECTION_COUNT_MAX; i++) {
        host_connection_type_t type = HostConnections[i].type;
        switch (type) {
            case HostConnectionType_Empty:
            case HostConnectionType_UsbHidRight:
            case HostConnectionType_UsbHidLeft:
            case HostConnectionType_Count:
            case HostConnectionType_NewBtHid:
                break;
            case HostConnectionType_UnregisteredBtHid:
                if (BtAddrEq(address, &HostConnections[i].bleAddress)) {
                    return HostKnown_Unregistered;
                }
                break;
            case HostConnectionType_Dongle:
            case HostConnectionType_BtHid:
                if (BtAddrEq(address, &HostConnections[i].bleAddress)) {
                    return HostKnown_Registered;
                }
                break;
        }
    }

    // Don't count new ble connections
    // Do check devices that are paired via settings - left, right, dongle
    for (int peerIdx = 0; peerIdx < PeerIdFirstHost; peerIdx++) {
        if (BtAddrEq(address, &Peers[peerIdx].addr)) {
            return HostKnown_Registered;
        }
    }

    return HostKnown_Unknown;
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
        HostConnection_SetSelectedConnection(connectionId);
        BtConn_ReserveConnections();
    }
    Connections_ReportState(connectionId);
    LastSelectedHostConnectionId = connectionId;
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
            LastSelectedHostConnectionId = i;
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

void HostConnections_SelectLastSelectedConnection(void) {
    if (LastSelectedHostConnectionId != ConnectionId_Invalid) {
        selectConnection(LastSelectedHostConnectionId);
    }
}

uint8_t HostConnections_NameToConnId(parser_context_t* ctx) {
    for (uint8_t i = ConnectionId_HostConnectionFirst; i <= ConnectionId_HostConnectionLast; i++) {
        if (Macros_CompareStringToken(ctx, HostConnection(i)->name)) {
            return i;
        }
    }

    return ConnectionId_Invalid;
}

void HostConnections_SelectByName(parser_context_t* ctx) {
    uint8_t connId = HostConnections_NameToConnId(ctx);

    if (connId != ConnectionId_Invalid) {
        selectConnection(connId);
    }
}

void HostConnections_SelectByHostConnIndex(uint8_t hostConnIndex) {
    uint8_t connId = ConnectionId_HostConnectionFirst + hostConnIndex;
    host_connection_t *hostConnection = HostConnection(connId);
    if (hostConnection && hostConnection->type != HostConnectionType_Empty) {
        selectConnection(connId);
    } else {
        LogU("Invalid host connection index: %d. Ignoring!\n", hostConnIndex);
        NotifyPrintf("Unassigned slot: %d.", hostConnIndex + 1);
    }
}

void HostConnections_ListKnownBleConnections() {
    printk("Known host connection ble addresses:\n");
    for (int i = 0; i < HOST_CONNECTION_COUNT_MAX; i++) {
        host_connection_type_t type = HostConnections[i].type;
        switch (type) {
            case HostConnectionType_Empty:
            case HostConnectionType_Count:
                break;
            case HostConnectionType_UsbHidRight:
            case HostConnectionType_UsbHidLeft:
                printk(" - %d '%.*s'\n", i, EXPAND_SEGMENT(HostConnections[i].name));
                break;
            case HostConnectionType_NewBtHid:
            case HostConnectionType_UnregisteredBtHid:
            case HostConnectionType_Dongle:
            case HostConnectionType_BtHid:
                printk(" - %d '%.*s': address: %s\n", i, EXPAND_SEGMENT(HostConnections[i].name), GetAddrString(&HostConnections[i].bleAddress));
                break;
        }
    }
}

void HostConnections_Reconnect() {
    connection_id_t connId = ActiveHostConnectionId;
    BtConn_DisconnectOne(connId);
    k_sleep(K_MSEC(100));
    HostConnections_SelectByHostConnIndex(connId - ConnectionId_HostConnectionFirst);
}

static void allocateUnregisteredHidId(const bt_addr_le_t *addr, connection_id_t connectionId) {
    host_connection_t* hostConnection = HostConnection(connectionId);
    host_connection_t* newConnectionTemplate = HostConnection(Connections_GetNewBtHidConnectionId());

    ASSERT(newConnectionTemplate);
    if (!newConnectionTemplate) {
        return;
    }

    ASSERT(Connections[connectionId].peerId == PeerIdUnknown);
    ASSERT(Connections[connectionId].state == ConnectionState_Disconnected);

    Bt_NewPairedDevice = true;
    hostConnection->type = HostConnectionType_UnregisteredBtHid;
    hostConnection->bleAddress = *addr;
    hostConnection->switchover = newConnectionTemplate->switchover;
    hostConnection->name = newConnectionTemplate->name;
}

/*
 * - Try to find existing allocation by Connections_GetConnectionIdByBtAddr(addr);
 * - If not found, search for a host connection slot of type Empty.
 * - If not found, return ConnectionId_Invalid.
 */
uint8_t HostConnections_AllocateConnectionIdForUnregisteredHid(const bt_addr_le_t *addr) {
    connection_id_t existingConnId = Connections_GetConnectionIdByHostAddr(addr);
    if (existingConnId != ConnectionId_Invalid) {
        return existingConnId;
    }

    for (uint8_t connectionId = ConnectionId_HostConnectionFirst; connectionId <= ConnectionId_HostConnectionLast; connectionId++) {
        host_connection_t *hostConnection = HostConnection(connectionId);
        if (hostConnection->type == HostConnectionType_Empty) {
            allocateUnregisteredHidId(addr, connectionId);
            return connectionId;
        }
    }

    // TODO: free connections that are not bonded anymore?

    return ConnectionId_Invalid;
}


void HostConnections_ClearConnectionByConnId(uint8_t connectionId) {
    host_connection_t *hostConnection = HostConnection(connectionId);

    if (hostConnection->type == HostConnectionType_UnregisteredBtHid) {
        BtPair_Unpair(hostConnection->bleAddress);
        NotifyPrintf("Slot %d cleared.", connectionId - ConnectionId_HostConnectionFirst + 1);

        hostConnection->type = HostConnectionType_Empty;
        memset(&hostConnection->bleAddress, 0, sizeof(bt_addr_le_t));
        hostConnection->name = (string_segment_t){ .start = NULL, .end = NULL };
        hostConnection->switchover = false;
    }
    else if (hostConnection->type == HostConnectionType_BtHid) {
        BtPair_Unpair(hostConnection->bleAddress);
        NotifyPrintf("Ble slot %d unpaired.", connectionId - ConnectionId_HostConnectionFirst + 1);
    }
    else if (hostConnection->type == HostConnectionType_Dongle) {
        BtPair_Unpair(hostConnection->bleAddress);
        NotifyPrintf("Dongle slot %d unpaired.", connectionId - ConnectionId_HostConnectionFirst + 1);
    }
    else if (hostConnection->type == HostConnectionType_Empty) {
        NotifyPrintf("Slot %d is empty.", connectionId - ConnectionId_HostConnectionFirst + 1);
    }
    else {
        NotifyPrintf("Unpairing %d not allowed.", connectionId - ConnectionId_HostConnectionFirst + 1);
    }
}

#endif


