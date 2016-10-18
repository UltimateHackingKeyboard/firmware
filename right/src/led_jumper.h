#ifndef __LED_JUMPER_H__
#define __LED_JUMPER_H__

// Includes:

    #include "fsl_gpio.h"

// Macros:

    #define LED_JUMPER_GPIO        GPIOC
    #define LED_JUMPER_PORT        PORTC
    #define LED_JUMPER_CLOCK       kCLOCK_PortC
    #define LED_JUMPER_PIN         4
    #define LED_JUMPER_IRQ         PORTC_IRQn
    #define LED_JUMPER_IRQ_HANDLER PORTC_IRQHandler

    #define LED_JUMPER_IS_ENABLED !GPIO_ReadPinInput(LED_JUMPER_GPIO, LED_JUMPER_PIN)

// Functions:

    extern void InitLedJumper();

#endif
