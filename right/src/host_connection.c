#include "host_connection.h"

#ifdef __ZEPHYR__

host_connection_t HostConnections[HOST_CONNECTION_COUNT_MAX] = {};

bool HostConnections_IsKnownBleAddress(const bt_addr_le_t *address) {
    for (int i = 0; i < HOST_CONNECTION_COUNT_MAX; i++) {
        switch (HostConnections[i].type) {
            case HostConnectionType_Empty:
            case HostConnectionType_UsbRight:
            case HostConnectionType_UsbLeft:
                break;
            case HostConnectionType_Dongle:
            case HostConnectionType_Ble:
                if (bt_addr_le_cmp(address, &HostConnections[i].bleAddress) == 0) {
                    return true;
                }
                break;
            default:
                break;
        }
    }
    return false;
}

#endif


