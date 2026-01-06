#ifndef __HOST_CONNECTION_H__
#define __HOST_CONNECTION_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "str_utils.h"

#ifdef __ZEPHYR__

    #include <zephyr/kernel.h>
    #include <zephyr/bluetooth/bluetooth.h>
    #include <zephyr/bluetooth/addr.h>

#else
    typedef struct {
        uint8_t val[6];
    } bt_addr_t;

    struct bt_addr_le {
        uint8_t      type;
        bt_addr_t    a;
    };

    typedef struct bt_addr_le bt_addr_le_t;
#endif

// Macros:

    #define SERIALIZED_HOST_CONNECTION_COUNT_MAX 22
    #define HOST_CONNECTION_COUNT_MAX (SERIALIZED_HOST_CONNECTION_COUNT_MAX+2)

    #define BLE_ADDRESS_LENGTH 6

// Typedefs:

    // these are serialized, don't change order!
    typedef enum {
        HostConnectionType_Empty = 0,
        HostConnectionType_UsbHidRight = 1,
        HostConnectionType_UsbHidLeft = 2,
        HostConnectionType_BtHid = 3,
        HostConnectionType_Dongle = 4,
        HostConnectionType_NewBtHid = 5,
        HostConnectionType_UnregisteredBtHid = 6,
        HostConnectionType_Count,
    } host_connection_type_t;

    typedef struct {
        host_connection_type_t type;
        bt_addr_le_t bleAddress;
        string_segment_t name;
        bool switchover;
    } ATTR_PACKED host_connection_t;

    typedef enum {
        HostKnown_Unknown = 0,
        HostKnown_Unregistered = 1,
        HostKnown_Registered = 2,
    } host_known_t;

// Variables:

    extern host_connection_t HostConnections[HOST_CONNECTION_COUNT_MAX];

// Functions:

    host_known_t HostConnections_IsKnownBleAddress(const bt_addr_le_t *address);
    host_connection_t* HostConnection(uint8_t connectionId);

    void HostConnections_ListKnownBleConnections();

    void HostConnections_SelectByHostConnIndex(uint8_t connectionId);
    void HostConnections_SelectLastConnection(void);
    void HostConnections_SelectLastSelectedConnection(void);
    void HostConnections_SelectNextConnection(void);
    void HostConnections_SelectPreviousConnection(void);
    void HostConnections_SelectByName(parser_context_t* ctx);
    void HostConnection_SetSelectedConnection(uint8_t connectionId);

    uint8_t HostConnections_NameToConnId(parser_context_t* ctx);

    void HostConnections_ClearConnectionByConnId(uint8_t connectionId);

    void HostConnections_Reconnect();

    uint8_t HostConnections_AllocateConnectionIdForUnregisteredHid(const bt_addr_le_t *addr);

#endif
