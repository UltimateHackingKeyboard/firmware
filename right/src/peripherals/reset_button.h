#ifndef _RESET_BUTTON_H_
#define _RESET_BUTTON_H_

// Includes:

    #include "fsl_gpio.h"

// Macros:

    #define RESET_BUTTON_GPIO        GPIOB
    #define RESET_BUTTON_PORT        PORTB
    #define RESET_BUTTON_CLOCK       kCLOCK_PortB
    #define RESET_BUTTON_PIN         2
    #define RESET_BUTTON_IRQ         PORTB_IRQn
    #define RESET_BUTTON_IRQ_HANDLER PORTB_IRQHandler

    #define RESET_BUTTON_IS_PRESSED !GPIO_ReadPinInput(RESET_BUTTON_GPIO, RESET_BUTTON_PIN)

// Functions:

    void InitResetButton(void);

#endif
