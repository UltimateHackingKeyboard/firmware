#ifndef __UART_PARSER_H__
#define __UART_PARSER_H__

// Includes:

    #include "attributes.h"
    #include <stdint.h>
    #include <stdbool.h>
    #include "crc16.h"
    #include "uart_defs.h"

// Macros:

//*2 for escapes

// Typedefs:

    typedef enum {
    UartControl_Ack = 1,
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
        uint8_t* txBuffer;
        uint16_t rxPosition;
        uint16_t txPosition;

        crc16_data_t crcState;

        bool escaping;
        bool receivingMessage;

    } uart_parser_t;

// Variables:

// Functions:

    void UartParser_InitParser( uart_parser_t* uartState, void (*receiveMessage)(void* state, uart_control_t messageKind, const uint8_t* rxBuffer, uint16_t len), void* userArg);
    void UartParser_SetRxBuffer(uart_parser_t *uartState, uint8_t* buffer);
    void UartParser_SetTxBuffer(uart_parser_t *uartState, uint8_t* buffer);

    void UartParser_StartMessage(uart_parser_t *uartState);
    void UartParser_AppendEscapedTxBytes(uart_parser_t *uartState, const uint8_t* data, uint16_t len);
    void UartParser_FinalizeMessage(uart_parser_t *uartState);


    void UartParser_ProcessIncomingBytes(void *state, const uint8_t* data, uint16_t len);


    // send messages

#endif // __UART_PARSER_H__
