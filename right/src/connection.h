#ifndef __CONNECTION_H__
#define __CONNECTION_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "str_utils.h"

// Macros:

    #define CONNECTION_COUNT_MAX 22
    #define BLE_ADDRESS_LENGTH 6

// Typedefs:

    typedef enum {
        ConnectionType_Empty,
        ConnectionType_UsbRight,
        ConnectionType_UsbLeft,
        ConnectionType_Ble,
        ConnectionType_Dongle,
    } connection_type_t;

    struct connection_t {
        connection_type_t type;
        uint8_t usb_id;
        uint8_t ble_id;
        uint8_t dongle_id;
    };

    typedef struct {
        connection_type_t type;
        uint8_t bleAddress[6];
        string_segment_t name;
    } ATTR_PACKED connection_t;


// Variables:


// Functions:

#endif
