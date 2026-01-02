#ifndef __SLAVE_PROTOCOL_HANDLER_H__
#define __SLAVE_PROTOCOL_HANDLER_H__

// Includes:

    #include "fsl_port.h"
    #include "crc16.h"
    #include "slave_protocol.h"

// Variables:

    extern i2c_message_t RxMessage;
    extern i2c_message_t TxMessage;

    extern bool SlaveProtocol_FreeUpdateAllowed;

// Functions:

    void SlaveRxHandler(const uint8_t* data);
    void SlaveTxHandler(const uint8_t* data);
    bool IsI2cRxTransaction(uint8_t commandId);

#endif
