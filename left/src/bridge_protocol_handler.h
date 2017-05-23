#ifndef __BRIDGE_PROTOCOL_HANDLER__
#define __BRIDGE_PROTOCOL_HANDLER__

// Includes:

    #include "fsl_port.h"

// Macros:

    #define BRIDGE_RX_BUFFER_SIZE 100
    #define BRIDGE_TX_BUFFER_SIZE 100

    #define PROTOCOL_RESPONSE_SUCCESS       0
    #define PROTOCOL_RESPONSE_GENERIC_ERROR 1

// Variables:

    uint8_t BridgeRxBuffer[BRIDGE_RX_BUFFER_SIZE];
    uint8_t BridgeTxBuffer[BRIDGE_TX_BUFFER_SIZE];
    uint8_t BridgeTxSize;

// Functions:

    extern void BridgeProtocolHandler(void);

#endif
