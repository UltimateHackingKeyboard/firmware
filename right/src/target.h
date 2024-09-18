#ifndef __TARGET_H__
#define __TARGET_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "str_utils.h"

// Macros:

    #define TARGET_COUNT_MAX 22
    #define BLE_ADDRESS_LENGTH 6

// Typedefs:

    typedef enum {
        TargetType_Empty,
        TargetType_UsbRight,
        TargetType_UsbLeft,
        TargetType_Ble,
        TargetType_Dongle,
    } target_type_t;

    struct target_t {
        target_type_t type;
        uint8_t usb_id;
        uint8_t ble_id;
        uint8_t dongle_id;
    };

    typedef struct {
        target_type_t type;
        uint8_t bleAddress[6];
        string_segment_t name;
    } ATTR_PACKED target_t;


// Variables:


// Functions:

#endif
