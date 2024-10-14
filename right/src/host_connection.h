#ifndef __HOST_CONNECTION_H__
#define __HOST_CONNECTION_H__

// Includes:

#ifdef __ZEPHYR__

    #include <stdint.h>
    #include <stdbool.h>
    #include "str_utils.h"

    #include <zephyr/kernel.h>
    #include <zephyr/bluetooth/bluetooth.h>
    #include <zephyr/bluetooth/addr.h>

// Macros:

    #define HOST_CONNECTION_COUNT_MAX 22

    #define BLE_ADDRESS_LENGTH 6

// Typedefs:

    typedef enum {
        HostConnectionType_Empty,
        HostConnectionType_UsbRight,
        HostConnectionType_UsbLeft,
        HostConnectionType_Ble,
        HostConnectionType_Dongle,
        HostConnectionType_Count,
    } host_connection_type_t;

    typedef struct {
        host_connection_type_t type;
        bt_addr_le_t bleAddress;
        string_segment_t name;
    } ATTR_PACKED host_connection_t;

// Variables:

    extern host_connection_t HostConnections[HOST_CONNECTION_COUNT_MAX];

// Functions:

    bool HostConnections_IsKnownBleAddress(const bt_addr_le_t *address);

#endif

#endif
