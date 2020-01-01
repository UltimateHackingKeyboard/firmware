#include "test_led.h"
#include "fsl_port.h"

extern void TestLed_Init(void)
{
    CLOCK_EnableClock(TEST_LED_CLOCK);
    PORT_SetPinMux(TEST_LED_PORT, TEST_LED_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(TEST_LED_GPIO, TEST_LED_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    TestLed_On();
}
