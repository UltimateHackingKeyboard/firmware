#ifndef __TEST_LED_H__
#define __TEST_LED_H__

// Includes:

    #include "fsl_gpio.h"

// Macros:

    #define LOGIC_LED_ON  0U
    #define LOGIC_LED_OFF 1U

    #define TEST_LED_GPIO      GPIOD
    #define TEST_LED_GPIO_PORT PORTD
    #define TEST_LED_GPIO_PIN  7U

    #define TEST_LED_INIT(output) GPIO_PinInit(TEST_LED_GPIO, TEST_LED_GPIO_PIN, \
                                              &(gpio_pin_config_t){kGPIO_DigitalOutput, (output)})
    #define TEST_LED_ON() GPIO_ClearPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN)
    #define TEST_LED_OFF() GPIO_SetPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN)
    #define TEST_LED_TOGGLE() GPIO_TogglePinsOutput(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN)

#endif
