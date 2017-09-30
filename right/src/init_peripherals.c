#include "fsl_common.h"
#include "fsl_port.h"
#include "config.h"
#include "peripherals/test_led.h"
#include "peripherals/reset_button.h"
#include "i2c.h"
#include "i2c_watchdog.h"
#include "peripherals/led_driver.h"
#include "peripherals/merge_sensor.h"
#include "led_pwm.h"
#include "slave_scheduler.h"
#include "peripherals/adc.h"
#include "init_peripherals.h"
#include "eeprom.h"
#include "microseconds/microseconds_pit.c"

void InitInterruptPriorities(void)
{
    NVIC_SetPriority(I2C0_IRQn, 1);
    NVIC_SetPriority(I2C1_IRQn, 1);
    NVIC_SetPriority(USB0_IRQn, 1);
}

void InitI2c(void)
{
     port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
        .openDrainEnable = kPORT_OpenDrainEnable
    };

    i2c_master_config_t masterConfig;
    I2C_MasterGetDefaultConfig(&masterConfig);
    uint32_t sourceClock;

    // Initialize main bus

    CLOCK_EnableClock(I2C_MAIN_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_MAIN_BUS_SCL_CLOCK);

    PORT_SetPinMux(I2C_MAIN_BUS_SCL_PORT, I2C_MAIN_BUS_SCL_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});
    PORT_SetPinMux(I2C_MAIN_BUS_SCL_PORT, I2C_MAIN_BUS_SDA_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SDA_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});

    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);

    GPIO_WritePinOutput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);

    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);

    GPIO_WritePinOutput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, 0);
    for (volatile uint32_t i=0; i<10000; i++);
    GPIO_WritePinOutput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, 1);
    for (volatile uint32_t i=0; i<10000; i++);

    pinConfig.mux = I2C_MAIN_BUS_MUX;
    PORT_SetPinConfig(I2C_MAIN_BUS_SDA_PORT, I2C_MAIN_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_MAIN_BUS_SCL_PORT, I2C_MAIN_BUS_SCL_PIN, &pinConfig);

    masterConfig.baudRate_Bps = I2C_MAIN_BUS_BAUD_RATE;
    sourceClock = CLOCK_GetFreq(I2C_MASTER_BUS_CLK_SRC);
    I2C_MasterInit(I2C_MAIN_BUS_BASEADDR, &masterConfig, sourceClock);

    // Initialize EEPROM bus

    CLOCK_EnableClock(I2C_EEPROM_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_EEPROM_BUS_SCL_CLOCK);

    pinConfig.mux = I2C_EEPROM_BUS_MUX;
    PORT_SetPinConfig(I2C_EEPROM_BUS_SDA_PORT, I2C_EEPROM_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_EEPROM_BUS_SCL_PORT, I2C_EEPROM_BUS_SCL_PIN, &pinConfig);

    masterConfig.baudRate_Bps = I2C_EEPROM_BUS_BAUD_RATE;
    sourceClock = CLOCK_GetFreq(I2C_EEPROM_BUS_CLK_SRC);
    I2C_MasterInit(I2C_EEPROM_BUS_BASEADDR, &masterConfig, sourceClock);
}

void InitPeripherals(void)
{
    InitInterruptPriorities();
    InitLedDriver();
    InitResetButton();
    InitMergeSensor();
    ADC_Init();
    InitI2c();
    InitTestLed();
    LedPwm_Init();
#ifdef I2C_WATCHDOG
    InitI2cWatchdog();
#endif
    EEPROM_Init();
    //microseconds_init();
}
