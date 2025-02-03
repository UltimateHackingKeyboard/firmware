#ifndef __UART_H__
#define __UART_H__

// Includes:

    #include "messenger.h"
    #include "link_protocol.h"

// Macros:

    #define UART_TIMEOUT 500
    #define UART_PING_DELAY 100
    #define UART_MAX_PACKET_LENGTH MAX_LINK_PACKET_LENGTH

// Variables:

    extern const struct device *uart_dev;
    extern uint16_t Uart_InvalidMessagesCounter;

// Functions:

    bool Uart_IsConnected();
    void Uart_SendPacket(const uint8_t* data, uint16_t len);
    void Uart_SendMessage(message_t* msg);
    bool Uart_Availability(messenger_availability_op_t total);
    void Uart_Enable();
    void InitUart(void);

#endif // __UART_H__
