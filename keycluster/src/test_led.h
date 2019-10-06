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
    #define TEST_LED_PIN   1

    static inline void TestLed_On(void)
    {
        GPIO_SetPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN);
    }

    static inline void TestLed_Off(void)
    {
        GPIO_ClearPinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN);
    }

    static inline void TestLed_Set(bool state)
    {
        GPIO_WritePinOutput(TEST_LED_GPIO, TEST_LED_PIN, state);
    }

    static inline void TestLed_Toggle(void)
    {
        GPIO_TogglePinsOutput(TEST_LED_GPIO, 1U << TEST_LED_PIN);
    }

// Functions:

    void TestLed_Init(void);

#endif
