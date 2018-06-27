#include "test_led.h"
#include "fsl_port.h"
#include "timer.h"

void TestLed_Init(void)
{
    CLOCK_EnableClock(TEST_LED_CLOCK);
    PORT_SetPinMux(TEST_LED_GPIO_PORT, TEST_LED_GPIO_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(TEST_LED_GPIO, TEST_LED_GPIO_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});
    TestLed_On();
}

void TestLed_Blink(uint8_t times)
{
    TestLed_Off();
    Timer_Delay(500);
    if (!times) {
        TestLed_On();
        Timer_Delay(500);
        TestLed_Off();
        Timer_Delay(500);
        return;
    }
    while (times--) {
        TestLed_On();
        Timer_Delay(100);
        TestLed_Off();
        Timer_Delay(100);
    }
    Timer_Delay(400);
}
