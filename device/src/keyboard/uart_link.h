#ifndef __UART_LINK_H__
#define __UART_LINK_H__

// Includes:

    #include "uart_defs.h"
    #include <zephyr/kernel.h>

// Macros:

//*2 for escapes

// Typedefs:

    typedef struct {
        void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len);
        void* userArg;

        const struct device *device;
        uint8_t *rxbuf;
        uint8_t rxbuf1[UART_MAX_SERIALIZED_MESSAGE_LENGTH];
        uint8_t rxbuf2[UART_MAX_SERIALIZED_MESSAGE_LENGTH];

        struct k_sem txControlBusy;
        bool enabled;
    } uart_link_t;

// Variables:

// Functions:


    void UartLink_Init(uart_link_t *uartState, const struct device* dev, void (*receiveBytes)(void* state, const uint8_t* data, uint16_t len), void* userArg);
    void UartLink_Enable(uart_link_t *uartState);
    void UartLink_Reset(uart_link_t *uartState);

    void UartLink_LockBusy(uart_link_t *uartState);
    int UartLink_Send(uart_link_t *uartState, uint8_t* data, uint16_t len);

#endif // __UART_LINK_H__
