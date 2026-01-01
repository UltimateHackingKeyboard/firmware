#ifndef __INIT_PERIPHERALS_H__
#define __INIT_PERIPHERALS_H__

// Macros:

    #define MODULE_OVER_UART true

    #define LED_DRIVER_SDB_PORT  PORTB
    #define LED_DRIVER_SDB_GPIO  GPIOB
    #define LED_DRIVER_SDB_CLOCK kCLOCK_PortB
    #define LED_DRIVER_SDB_PIN   1

// Functions:

    void InitPeripherals(void);
    void initI2c(void);
    void initUart(void);
    void uartSendFaByte(void);

#endif
