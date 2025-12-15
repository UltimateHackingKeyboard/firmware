#ifndef __UART_H__
#define __UART_H__

// Includes:

    #include "messenger.h"
    #include "link_protocol.h"
    #include "pin_wiring.h"
    #include "uart_defs.h"

// Macros:

    #define UART_PING_DELAY 100

// Variables:

    extern const struct device *uart_dev;
    extern uint16_t Uart_InvalidMessagesCounter;

// Functions:

    // void Uart_ControlMessage(const pin_wiring_dev_t *device, const uint8_t* data, uint16_t len);
    int Uart_SendModuleMessage(i2c_message_t* msg);
    void UartBridge_SendMessage(message_t* msg);
    void UartBridge_Enable();
    void InitUartBridge(void);



#endif // __UART_H__
