#ifndef __SLAVE_PROTOCOL_HANDLER_H__
#define __SLAVE_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_port.h"
    #include "crc16.h"
    #include "slave_protocol.h"

// Macros:

    #define PROTOCOL_RESPONSE_SUCCESS       0
    #define PROTOCOL_RESPONSE_GENERIC_ERROR 1

    #define KEY_STATE_SIZE (MODULE_KEY_COUNT/8 + (MODULE_KEY_COUNT % 8 ? 1 : 0))

// Variables:

    extern i2c_message_t RxMessage;
    extern i2c_message_t TxMessage;

// Functions:

    void SlaveRxHandler(void);
    void SlaveTxHandler(void);

#endif
