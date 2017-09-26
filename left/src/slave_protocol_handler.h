#ifndef __SLAVE_PROTOCOL_HANDLER_H__
#define __SLAVE_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_port.h"
    #include "crc16.h"
    #include "slave_protocol.h"

// Macros:

    #define PROTOCOL_RESPONSE_SUCCESS       0
    #define PROTOCOL_RESPONSE_GENERIC_ERROR 1

    #define LEFT_KEYBOARD_HALF_KEY_COUNT (KEYBOARD_MATRIX_COLS_NUM*KEYBOARD_MATRIX_ROWS_NUM)
    #define KEY_STATE_SIZE (LEFT_KEYBOARD_HALF_KEY_COUNT/8 + 1)

// Variables:

    extern i2c_message_t rxMessage;
    extern i2c_message_t txMessage;

// Functions:

    void SlaveRxHandler(void);
    void SlaveTxHandler(void);

#endif
