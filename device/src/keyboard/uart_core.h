#ifndef __UART_CORE_H__
#define __UART_CORE_H__

// Includes:

    #include "uart.h"

// Macros:

//*2 for escapes

    #define UART_CORE_SLOTS 1
    #define UART_CORE_CRC_BUF_LEN 4
    #define UART_CORE_START_END_BYTE_LEN 2
    #define UART_CORE_TX_BUF_SIZE UART_MAX_PACKET_LENGTH*2+UART_CORE_START_END_BYTE_LEN+UART_CORE_CRC_BUF_LEN
    #define UART_CORE_BUF_SIZE UART_CORE_TX_BUF_SIZE

// Typedefs:

    typedef struct {
        void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len);
        void* userArg;

        const struct device *device;
        uint8_t txBuffer[UART_CORE_TX_BUF_SIZE];
        uint16_t txPosition;
        uint8_t *rxbuf;
        uint8_t rxbuf1[UART_CORE_BUF_SIZE];
        uint8_t rxbuf2[UART_CORE_BUF_SIZE];

        struct k_sem txControlBusy;
        bool enabled;
    } uart_core_t;

// Variables:

// Functions:


    void UartCore_Init(uart_core_t *uartState, const struct device* dev, void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len), void* userArg);
    void UartCore_Enable(uart_core_t *uartState);
    void UartCore_ResetUart(uart_core_t *uartState);

    void UartCore_TakeControl(uart_core_t *uartState);
    void UartCore_Send(uart_core_t *uartState, uint8_t* data, uint16_t len);
    void UartCore_AppendTxByte(uart_core_t *uartState, uint8_t byte);
    void UartCore_SetEscapedTxByte(uart_core_t *uartState, uint8_t idx, uint8_t byte, uint8_t escape);

#endif // __UART_CORE_H__
