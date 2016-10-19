#ifndef __ISO_JUMPER_H__
#define __ISO_JUMPER_H__

// Includes:

    #include "fsl_gpio.h"

// Macros:

    #define ISO_JUMPER_INPUT_GPIO        GPIOB
    #define ISO_JUMPER_INPUT_PORT        PORTB
    #define ISO_JUMPER_INPUT_CLOCK       kCLOCK_PortB
    #define ISO_JUMPER_INPUT_PIN         13
    #define ISO_JUMPER_INPUT_IRQ         PORTB_IRQn
    #define ISO_JUMPER_INPUT_IRQ_HANDLER PORTB_IRQHandler

    #define ISO_JUMPER_OUTPUT_GPIO        GPIOA
    #define ISO_JUMPER_OUTPUT_PORT        PORTA
    #define ISO_JUMPER_OUTPUT_CLOCK       kCLOCK_PortA
    #define ISO_JUMPER_OUTPUT_PIN         6
    #define ISO_JUMPER_OUTPUT_IRQ         PORTA_IRQn
    #define ISO_JUMPER_OUTPUT_IRQ_HANDLER PORTA_IRQHandler

    #define ISO_JUMPER_IS_ENABLED !GPIO_ReadPinInput(ISO_JUMPER_INPUT_GPIO, ISO_JUMPER_INPUT_PIN)

// Functions:

    extern void InitIsoJumper();

#endif
