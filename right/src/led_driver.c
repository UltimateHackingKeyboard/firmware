#include "led_driver.h"
#include "i2c_addresses.h"

void LedDriver_WriteRegister(uint8_t i2cAddress, uint8_t reg, uint8_t val)
{
    uint8_t buffer[] = {reg, val};
    I2cWrite(I2C_MAIN_BUS_BASEADDR, i2cAddress, buffer, sizeof(buffer));
}

void LedDriver_InitAllLeds(char isEnabled)
{
    CLOCK_EnableClock(LED_DRIVER_SDB_CLOCK);
    PORT_SetPinMux(LED_DRIVER_SDB_PORT, LED_DRIVER_SDB_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, 1);
}
