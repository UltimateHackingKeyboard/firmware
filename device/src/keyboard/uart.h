#ifndef __UART_H__
#define __UART_H__

// Macros:

    #define UART_TIMEOUT 2000
    #define UART_MAX_PACKET_LENGTH 128

// Variables:

    extern const struct device *uart_dev;

// Functions:

    bool Uart_IsConnected();
    void Uart_SendPacket(const uint8_t* data, uint16_t len);
    void InitUart(void);

#endif // __UART_H__
