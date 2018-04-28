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
#include "peripherals/adc.h"
#include "init_peripherals.h"
#include "eeprom.h"
#include "timer.h"
#include "key_debouncer.h"
#include "usb_api.h"
#include "slave_scheduler.h"
#include "bootloader/wormhole.h"

bool IsBusPalOn;
volatile uint32_t I2cMainBusRequestedBaudRateBps = I2C_MAIN_BUS_NORMAL_BAUD_RATE;
volatile uint32_t I2cMainBusActualBaudRateBps;

static void initBusPalState(void) {
    IsBusPalOn = Wormhole.magicNumber == WORMHOLE_MAGIC_NUMBER && Wormhole.enumerationMode == EnumerationMode_BusPal;
    if (IsBusPalOn) {
        Wormhole.magicNumber = 0;
        I2cMainBusRequestedBaudRateBps = I2C_MAIN_BUS_BUSPAL_BAUD_RATE;
    }
}

static void initInterruptPriorities(void)
{
    NVIC_SetPriority(PIT_I2C_WATCHDOG_IRQ_ID,  1);
    NVIC_SetPriority(PIT_TIMER_IRQ_ID,         2);
    NVIC_SetPriority(PIT_KEY_SCANNER_IRQ_ID,   3);
    NVIC_SetPriority(PIT_KEY_DEBOUNCER_IRQ_ID, 3);
    NVIC_SetPriority(I2C_MAIN_BUS_IRQ_ID,      3);
    NVIC_SetPriority(I2C_EEPROM_BUS_IRQ_ID,    3);
    NVIC_SetPriority(USB_IRQ_ID,               3);
}

static void delay(void)
{
    for (volatile uint32_t i=0; i<62; i++);
}

static void recoverI2c(void)
{
    PORT_SetPinMux(I2C_MAIN_BUS_SDA_PORT, I2C_MAIN_BUS_SDA_PIN, kPORT_MuxAsGpio);
    PORT_SetPinMux(I2C_MAIN_BUS_SCL_PORT, I2C_MAIN_BUS_SCL_PIN, kPORT_MuxAsGpio);
    GPIO_PinInit(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});

    bool isOn = true;
    for (int i=0; i<20; i++) {
        GPIO_PinInit(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, &(gpio_pin_config_t){kGPIO_DigitalInput});
        bool isSdaHigh = GPIO_ReadPinInput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN);
        GPIO_PinInit(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});

        if (isSdaHigh) {
            return;
        }

        GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, isOn);
        delay();
        isOn = !isOn;
    }

    GPIO_WritePinOutput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, 0);
    delay();
    GPIO_WritePinOutput(I2C_MAIN_BUS_SCL_GPIO, I2C_MAIN_BUS_SCL_PIN, 1);
    delay();
    GPIO_WritePinOutput(I2C_MAIN_BUS_SDA_GPIO, I2C_MAIN_BUS_SDA_PIN, 1);
    delay();
}

static void initI2cMainBus(void)
{
    CLOCK_EnableClock(I2C_MAIN_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_MAIN_BUS_SCL_CLOCK);

    recoverI2c();

    port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
        .openDrainEnable = kPORT_OpenDrainEnable,
        .mux = I2C_MAIN_BUS_MUX,
    };

    PORT_SetPinConfig(I2C_MAIN_BUS_SDA_PORT, I2C_MAIN_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_MAIN_BUS_SCL_PORT, I2C_MAIN_BUS_SCL_PIN, &pinConfig);

    i2c_master_config_t masterConfig;
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2cMainBusRequestedBaudRateBps;
    uint32_t sourceClock = CLOCK_GetFreq(I2C_MAIN_BUS_CLK_SRC);
    I2C_MasterInit(I2C_MAIN_BUS_BASEADDR, &masterConfig, sourceClock);
    I2cMainBusActualBaudRateBps = I2C_ActualBaudRate;
}

void ReinitI2cMainBus(void)
{
    I2C_MasterDeinit(I2C_MAIN_BUS_BASEADDR);
    initI2cMainBus();
    InitSlaveScheduler();
}

static void initI2cEepromBus(void)
{
    port_pin_config_t pinConfig = {
       .pullSelect = kPORT_PullUp,
       .openDrainEnable = kPORT_OpenDrainEnable,
       .mux = I2C_EEPROM_BUS_MUX,
   };

   CLOCK_EnableClock(I2C_EEPROM_BUS_SDA_CLOCK);
   CLOCK_EnableClock(I2C_EEPROM_BUS_SCL_CLOCK);

   PORT_SetPinConfig(I2C_EEPROM_BUS_SDA_PORT, I2C_EEPROM_BUS_SDA_PIN, &pinConfig);
   PORT_SetPinConfig(I2C_EEPROM_BUS_SCL_PORT, I2C_EEPROM_BUS_SCL_PIN, &pinConfig);

   i2c_master_config_t masterConfig;
   I2C_MasterGetDefaultConfig(&masterConfig);
   masterConfig.baudRate_Bps = I2C_EEPROM_BUS_BAUD_RATE;
   uint32_t sourceClock = CLOCK_GetFreq(I2C_EEPROM_BUS_CLK_SRC);
   I2C_MasterInit(I2C_EEPROM_BUS_BASEADDR, &masterConfig, sourceClock);
}

static void initI2c(void)
{
    initI2cMainBus();
    initI2cEepromBus();
}

void InitPeripherals(void)
{
    initBusPalState();
    initInterruptPriorities();
    Timer_Init();
    InitLedDriver();
    InitResetButton();
    InitMergeSensor();
    ADC_Init();
    initI2c();
    InitTestLed();
    LedPwm_Init();
    InitI2cWatchdog();
    InitKeyDebouncer();
    EEPROM_Init();
}
