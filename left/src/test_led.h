#ifndef __TEST_LED_H__
#define __TEST_LED_H__

// Includes:

    #include "fsl_gpio.h"

// Macros:

    #define LOGIC_LED_ON  0U
    #define LOGIC_LED_OFF 1U

    #define TEST_LED_GPIO      GPIOA
    #define TEST_LED_PORT PORTA
    #define TEST_LED_CLOCK     kCLOCK_PortA
    #define TEST_LED_PIN  12

    #define TEST_LED_ON() GPIO_ClearPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN)
    #define TEST_LED_OFF() GPIO_SetPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN)
    #define TEST_LED_TOGGLE() GPIO_TogglePinsOutput(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN)

// Functions:

    extern void InitTestLed();

#endif
