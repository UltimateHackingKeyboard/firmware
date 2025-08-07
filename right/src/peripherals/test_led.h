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

    static inline void TestLed_On(void)
    {
        GPIO_PortSet(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN);
    }

    static inline void TestLed_Off(void)
    {
        GPIO_PortClear(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN);
    }

    static inline void TestLed_Set(bool state)
    {
        GPIO_PinWrite(TEST_LED_GPIO, TEST_LED_GPIO_PIN, state);
    }

    static inline void TestLed_Toggle(void)
    {
        GPIO_PortToggle(TEST_LED_GPIO, 1U << TEST_LED_GPIO_PIN);
    }

// Functions:

    void TestLed_Init(void);
    void TestLed_Blink(uint8_t times);

#endif
