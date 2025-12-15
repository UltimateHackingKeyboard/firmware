#ifndef __UART_DEFS_H__
#define __UART_DEFS_H__

// Includes:

    #include "attributes.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include "crc16.h"


    #if !defined(DEVICE_ID)
        // modules don't have that much memory
        #define UART_MAX_PAYLOAD_LENGTH 128
    #else
        // to comply with BLE
        #define UART_MAX_PAYLOAD_LENGTH 244
    #endif

    #define UART_LINK_SLOTS 1
    #define UART_LINK_CRC_BUF_LEN 4
    #define UART_LINK_START_END_BYTE_LEN 2

    // 244 = max BLE payload length
    #define UART_MAX_SERIALIZED_MESSAGE_LENGTH (UART_MAX_PAYLOAD_LENGTH*2 + UART_LINK_START_END_BYTE_LEN + UART_LINK_CRC_BUF_LEN)

    #define UART_BRIDGE_TIMEOUT 500

    #define UART_MODULE_PING_INTERVAL_MS 500
    #define UART_MODULE_TIMEOUT_MS (UART_MODULE_PING_INTERVAL_MS*4)

// Macros:

//*2 for escapes

// Typedefs:

// Variables:

// Functions:

    // send messages

#endif // __UART_DEFS_H__
