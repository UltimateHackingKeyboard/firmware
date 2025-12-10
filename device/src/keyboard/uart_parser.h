#ifndef __UART_PARSER_H__
#define __UART_PARSER_H__

// Includes:

    #include "uart_bridge.h"
    #include "attributes.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include "uart_link.h"
    #include "crc16.h"

// Macros:

//*2 for escapes

// Typedefs:

typedef enum {
    UartControl_Ack,
    UartControl_Nack,
    UartControl_Ping,
    UartControl_ValidMessage,
    UartControl_InvalidMessage,
    UartControl_Unexpected,
} uart_control_t;

typedef enum {
    UartControlByte_Start = 0b01010100,
    UartControlByte_End = 0b01010101,
    UartControlByte_Escape = 0b01010110,
    UartControlByte_Ack = 0b01010111,
    UartControlByte_Nack = 0b01011000,
    UartControlByte_Ping = 0b01011001,
} uart_control_byte_t;

typedef struct {
    void* userArg;
    void (*receiveMessage)(void* state, uart_control_t messageKind, const uint8_t* data, uint16_t len);

    uint8_t* rxBuffer;
    uint16_t rxPosition;

    crc16_data_t crcState;

    uint8_t txBuffer[UART_LINK_TX_BUF_SIZE];
    uint16_t txPosition;

    bool escaping;
    bool receivingMessage;

    uart_link_t* core;

} uart_parser_t;

// Variables:

// Functions:

    void UartParser_InitParser( uart_parser_t* uartState, uart_link_t * core, void (*receiveMessage)(void* state, uart_control_t messageKind, const uint8_t* rxBuffer, uint16_t len), void* userArg);
    void UartParser_SetBuffer(uart_parser_t *uartState, uint8_t* buffer);

    void UartParser_StartMessage(uart_parser_t *uartState);
    void UartParser_FinalizeMessage(uart_parser_t *uartState);

    void UartParser_AppendRawTxByte(uart_parser_t *uartState, uint8_t byte);
    void UartParser_AppendEscapedTxBytes(uart_parser_t *uartState, const uint8_t* data, uint16_t len);
    void UartParser_SetEscapedTxByte(uart_parser_t *uartState, uint8_t idx, uint8_t byte, uint8_t escape);

    void UartParser_ProcessIncomingBytes(void *state, const uint8_t* data, uint16_t len);


    // send messages

#endif // __UART_PARSER_H__
