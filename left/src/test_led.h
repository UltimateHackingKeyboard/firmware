#ifndef __TEST_LED_H__
#define __TEST_LED_H__

// Includes:

    #include "fsl_gpio.h"

// Macros:

    #define LOGIC_LED_ON  0U
    #define LOGIC_LED_OFF 1U

    #define TEST_LED_GPIO  GPIOB
    #define TEST_LED_PORT  PORTB
    #define TEST_LED_CLOCK kCLOCK_PortB
    #define TEST_LED_PIN   13

    #define TEST_LED_ON() GPIO_SetPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN)
    #define TEST_LED_OFF() GPIO_ClearPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN)
    #define TEST_LED_SET(state) GPIO_WritePinOutput(TEST_LED_GPIO, TEST_LED_PIN, (state))
    #define TEST_LED_TOGGLE() GPIO_TogglePinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN)

// Functions:

    void InitTestLed(void);

#endif
