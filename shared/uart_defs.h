#ifndef __UART_DEFS_H__
#define __UART_DEFS_H__

// Includes:

    #include "attributes.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include "crc16.h"

// Macros:


    // modules have very limited RAM, so keep low; Also, keep it
    #define UART_MAX_MODULE_PAYLOAD_LENGTH SLAVE_PROTOCOL_MAX_PAYLOAD_LENGTH
    // Max deserialized bridge message length. Must equal MAX_LINK_PACKET_LENGTH
    // (link_protocol.h) - the bridge rx buffer is a messenger-queue region of
    // that size, and messages travel interchangeably over UART and BLE. Kept
    // small (rather than the BLE-max 244) to save RAM and cut BLE latency.
    #define UART_MAX_BRIDGE_PAYLOAD_LENGTH 128

    #define UART_CRC_LEN 2

    #define UART_LINK_SLOTS 1
    #define UART_LINK_CRC_BUF_LEN 4
    #define UART_LINK_START_END_BYTE_LEN 2

    #define UART_MAX_SERIALIZED_LENGTH(PAYLOAD_LENGTH) (PAYLOAD_LENGTH*2 + UART_LINK_START_END_BYTE_LEN + UART_LINK_CRC_BUF_LEN)

    #define UART_MAX_BRIDGE_SERIALIZED_MESSAGE_LENGTH UART_MAX_SERIALIZED_LENGTH(UART_MAX_BRIDGE_PAYLOAD_LENGTH)
    #define UART_MAX_MODULE_SERIALIZED_MESSAGE_LENGTH UART_MAX_SERIALIZED_LENGTH(UART_MAX_MODULE_PAYLOAD_LENGTH)
    #define UART_MAX_SERIALIZED_MESSAGE_LENGTH MAX(UART_MAX_BRIDGE_SERIALIZED_MESSAGE_LENGTH, UART_MAX_MODULE_SERIALIZED_MESSAGE_LENGTH)

    #define UART_BRIDGE_PING_INTERVAL 200
    #define UART_BRIDGE_TIMEOUT 700

    // One byte-time on the wire at 115200-8N1 (~87us), rounded up. A physical property of the
    // link, so it outlives any particular power scheme.
    #define UART_BYTE_TIME_US 90
    #define UART_FRAME_WIRE_TIME_MS(BYTES) (((BYTES) * UART_BYTE_TIME_US + 999) / 1000)

    #define UART_RESEND_DELAY 7
    #define UART_RESEND_COUNT 5

    #define UART_MODULE_PING_INTERVAL_MS 500
    #define UART_MODULE_TIMEOUT_MS (UART_MODULE_PING_INTERVAL_MS*4)

#endif // __UART_DEFS_H__
