#include "peripherals/led_driver.h"
#include "i2c_addresses.h"

void InitLedDriver(void)
{
    CLOCK_EnableClock(LED_DRIVER_SDB_CLOCK);
    PORT_SetPinMux(LED_DRIVER_SDB_PORT, LED_DRIVER_SDB_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, 1);

#if DEVICE_ID == DEVICE_ID_UHK60V2
    CLOCK_EnableClock(LED_DRIVER_IICRST_CLOCK);
    PORT_SetPinMux(LED_DRIVER_IICRST_PORT, LED_DRIVER_IICRST_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(LED_DRIVER_IICRST_GPIO, LED_DRIVER_IICRST_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(LED_DRIVER_IICRST_GPIO, LED_DRIVER_IICRST_PIN, 0);
#endif
}
