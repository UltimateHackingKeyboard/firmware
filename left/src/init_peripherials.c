#include "fsl_common.h"
#include "fsl_port.h"
#include "test_led.h"
#include "init_peripherials.h"
#include "i2c_addresses.h"
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "i2c.h"
#include "led_pwm.h"

void InitI2c() {
    port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
    };

    // Initialize main bus

    CLOCK_EnableClock(I2C_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_BUS_SCL_CLOCK);

    pinConfig.mux = I2C_BUS_MUX;
    PORT_SetPinConfig(I2C_BUS_SDA_PORT, I2C_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_BUS_SCL_PORT, I2C_BUS_SCL_PIN, &pinConfig);

    i2c_slave_config_t slaveConfig;
    I2C_SlaveGetDefaultConfig(&slaveConfig);
    slaveConfig.slaveAddress = I2C_ADDRESS_LEFT_KEYBOARD_HALF;
    slaveConfig.addressingMode = kI2C_Address7bit / kI2C_RangeMatch;
    I2C_SlaveInit(I2C_BUS_BASEADDR, &slaveConfig);
}

void InitLedDriver() {
    CLOCK_EnableClock(LED_DRIVER_SDB_CLOCK);
    PORT_SetPinMux(LED_DRIVER_SDB_PORT, LED_DRIVER_SDB_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 0});
    GPIO_WritePinOutput(LED_DRIVER_SDB_GPIO, LED_DRIVER_SDB_PIN, 1);
}

void InitPeripherials(void)
{
    InitLedDriver();
    InitTestLed();
    LedPwm_Init();
    InitI2c();
}
