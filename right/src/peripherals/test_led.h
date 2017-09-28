#ifndef __TEST_LED_H__
#define __TEST_LED_H__

// Includes:

    #include "fsl_gpio.h"

// Macros:

    #define LOGIC_LED_ON  0U
    #define LOGIC_LED_OFF 1U

    #define TEST_LED_GPIO      GPIOD
    #define TEST_LED_GPIO_PORT PORTD
    #define TEST_LED_CLOCK kCLOCK_PortD
    #define TEST_LED_GPIO_PIN  7U

    #define TEST_LED_ON() GPIO_SetPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN)
    #define TEST_LED_OFF() GPIO_ClearPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN)
    #define TEST_LED_SET(state) GPIO_WritePinOutput(TEST_LED_GPIO, TEST_LED_GPIO_PIN, (state))
    #define TEST_LED_TOGGLE() GPIO_TogglePinsOutput(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN)

// Functions:

    void InitTestLed(void);

#endif
