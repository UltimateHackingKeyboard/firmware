#ifndef __INIT_H__
#define __INIT_H__

// Macros:

    #define LED_DRIVER_SDB_PORT  PORTB
    #define LED_DRIVER_SDB_GPIO  GPIOB
    #define LED_DRIVER_SDB_CLOCK kCLOCK_PortB
    #define LED_DRIVER_SDB_PIN   1

// Functions:

    void InitPeripherials();

#endif
