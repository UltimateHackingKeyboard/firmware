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
    GPIO_PinInit(GPIOA, LED_DRIVER_SDB_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, 1);

    LedDriver_SetAllLedsTo(isEnabled ? 0xFF : 0x00);
}

void LedDriver_SetAllLedsTo(uint8_t val)
{
    uint8_t ledDriverAddresses[] = {I2C_ADDRESS_LED_DRIVER_LEFT, I2C_ADDRESS_LED_DRIVER_RIGHT};

    for (uint8_t addressId=0; addressId<sizeof(ledDriverAddresses); addressId++) {
        uint8_t address = ledDriverAddresses[addressId];
        LedDriver_WriteRegister(address, LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_FUNCTION);
        LedDriver_WriteRegister(address, LED_DRIVER_REGISTER_SHUTDOWN, SHUTDOWN_MODE_NORMAL);
        LedDriver_WriteRegister(address, LED_DRIVER_REGISTER_FRAME, LED_DRIVER_FRAME_1);

        uint8_t i;
        for (i=FRAME_REGISTER_PWM_FIRST; i<=FRAME_REGISTER_PWM_LAST; i++) {
            LedDriver_WriteRegister(address, i, val);
        }
        for (i=FRAME_REGISTER_LED_CONTROL_FIRST; i<=FRAME_REGISTER_LED_CONTROL_LAST; i++) {
            LedDriver_WriteRegister(address, i, 0xff);
        }
        for (i=FRAME_REGISTER_BLINK_CONTROL_FIRST; i<=FRAME_REGISTER_BLINK_CONTROL_LAST; i++) {
            LedDriver_WriteRegister(address, i, 0x00);
        }
    }
}
