#ifndef __UART_H__
#define __UART_H__

// Includes:

    #include "messenger.h"
    #include "link_protocol.h"
    #include "pin_wiring.h"
    #include "uart_defs.h"

// Macros:

// Variables:

    extern const struct device *uart_dev;
    extern uint16_t Uart_InvalidMessagesCounter;

// Functions:

    int Uart_SendModuleMessage(i2c_message_t* msg);
    int UartBridge_SendMessage(message_t* msg);
    void UartBridge_Enable();
    void InitUartBridge(void);

    // Take the bridge down for deep sleep: drain any in-flight TX, tear RX down and
    // pm-suspend the UARTE, which otherwise holds HFCLK for as long as RX is armed.
    // No-op when this routing has no bridge uart.
    void UartBridge_Suspend(void);
    void UartBridge_Resume(void);

#endif // __UART_H__
