#ifndef __HOST_CONNECTION_H__
#define __HOST_CONNECTION_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "str_utils.h"

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
    } host_connection_type_t;

    struct host_connection_t {
        host_connection_type_t type;
        uint8_t usb_id;
        uint8_t ble_id;
        uint8_t dongle_id;
    };

    typedef struct {
        host_connection_type_t type;
        uint8_t bleAddress[6];
        string_segment_t name;
    } ATTR_PACKED host_connection_t;


// Variables:


// Functions:

#endif
