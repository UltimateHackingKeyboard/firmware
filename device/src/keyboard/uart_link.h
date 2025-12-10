#ifndef __UART_LINK_H__
#define __UART_LINK_H__

// Includes:

    #include "uart_bridge.h"

// Macros:

//*2 for escapes

    #define UART_LINK_SLOTS 1
    #define UART_LINK_CRC_BUF_LEN 4
    #define UART_LINK_START_END_BYTE_LEN 2
    #define UART_LINK_TX_BUF_SIZE UART_MAX_PACKET_LENGTH*2+UART_LINK_START_END_BYTE_LEN+UART_LINK_CRC_BUF_LEN
    #define UART_LINK_BUF_SIZE UART_LINK_TX_BUF_SIZE

// Typedefs:

    typedef struct {
        void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len);
        void* userArg;

        const struct device *device;
        uint8_t *rxbuf;
        uint8_t rxbuf1[UART_LINK_BUF_SIZE];
        uint8_t rxbuf2[UART_LINK_BUF_SIZE];

        struct k_sem txControlBusy;
        bool enabled;
    } uart_link_t;

// Variables:

// Functions:


    void UartLink_Init(uart_link_t *uartState, const struct device* dev, void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len), void* userArg);
    void UartLink_Enable(uart_link_t *uartState);
    void UartLink_ResetUart(uart_link_t *uartState);

    void UartLink_TakeControl(uart_link_t *uartState);
    int UartLink_Send(uart_link_t *uartState, uint8_t* data, uint16_t len);

#endif // __UART_LINK_H__
