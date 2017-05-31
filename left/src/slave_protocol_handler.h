#ifndef __SLAVE_PROTOCOL_HANDLER_H__
#define __SLAVE_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_port.h"
    #include "crc16.h"

// Macros:

    #define SLAVE_RX_BUFFER_SIZE 100
    #define SLAVE_TX_BUFFER_SIZE 100

    #define PROTOCOL_RESPONSE_SUCCESS       0
    #define PROTOCOL_RESPONSE_GENERIC_ERROR 1

    #define LEFT_KEYBOARD_HALF_KEY_COUNT (KEYBOARD_MATRIX_COLS_NUM*KEYBOARD_MATRIX_ROWS_NUM)
    #define KEY_STATE_SIZE (LEFT_KEYBOARD_HALF_KEY_COUNT/8 + 1)
    #define KEY_STATE_BUFFER_SIZE (KEY_STATE_SIZE + CRC16_HASH_LENGTH)

// Variables:

    uint8_t SlaveRxBuffer[SLAVE_RX_BUFFER_SIZE];
    uint8_t SlaveTxBuffer[SLAVE_TX_BUFFER_SIZE];
    uint8_t SlaveTxSize;

// Functions:

    extern void SlaveProtocolHandler(void);

#endif
