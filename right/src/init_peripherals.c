#include "fsl_common.h"
#include "fsl_port.h"
#include "fsl_pit.h"
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
#include "slave_protocol.h"
#include "timer.h"
#include "usb_api.h"
#include "slave_scheduler.h"
#include "bootloader/wormhole.h"
#include "module/uart.h"

bool IsBusPalOn;
volatile uint32_t I2cMainBusRequestedBaudRateBps = I2C_MAIN_BUS_NORMAL_BAUD_RATE;
volatile uint32_t I2cMainBusActualBaudRateBps;

static i2c_bus_t i2cMainBus = {
    .baseAddr = I2C_MAIN_BUS_BASEADDR,
    .clockSrc = I2C_MAIN_BUS_CLK_SRC,
    .mux = I2C_MAIN_BUS_MUX,

    .sdaClock = I2C_MAIN_BUS_SDA_CLOCK,
    .sdaGpio = I2C_MAIN_BUS_SDA_GPIO,
    .sdaPort = I2C_MAIN_BUS_SDA_PORT,
    .sdaPin = I2C_MAIN_BUS_SDA_PIN,

    .sclClock = I2C_MAIN_BUS_SCL_CLOCK,
    .sclGpio = I2C_MAIN_BUS_SCL_GPIO,
    .sclPort = I2C_MAIN_BUS_SCL_PORT,
    .sclPin = I2C_MAIN_BUS_SCL_PIN,
};

static i2c_bus_t i2cEepromBus = {
    .baseAddr = I2C_EEPROM_BUS_BASEADDR,
    .clockSrc = I2C_EEPROM_BUS_CLK_SRC,
    .mux = I2C_EEPROM_BUS_MUX,

    .sdaClock = I2C_EEPROM_BUS_SDA_CLOCK,
    .sdaGpio = I2C_EEPROM_BUS_SDA_GPIO,
    .sdaPort = I2C_EEPROM_BUS_SDA_PORT,
    .sdaPin = I2C_EEPROM_BUS_SDA_PIN,

    .sclClock = I2C_EEPROM_BUS_SCL_CLOCK,
    .sclGpio = I2C_EEPROM_BUS_SCL_GPIO,
    .sclPort = I2C_EEPROM_BUS_SCL_PORT,
    .sclPin = I2C_EEPROM_BUS_SCL_PIN,
};

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
    NVIC_SetPriority(I2C_EEPROM_BUS_IRQ_ID,    0);
    NVIC_SetPriority(PIT_TIMER_IRQ_ID,         3);
    NVIC_SetPriority(I2C_MAIN_BUS_IRQ_ID,      4);
    NVIC_SetPriority(USB_IRQ_ID,               4);
}

static void delay(void)
{
    for (volatile uint32_t i=0; i<62; i++);
}

static void recoverI2cBus(i2c_bus_t *i2cBus)
{
    PORT_SetPinMux(i2cBus->sdaPort, i2cBus->sdaPin, kPORT_MuxAsGpio);
    PORT_SetPinMux(i2cBus->sclPort, i2cBus->sclPin, kPORT_MuxAsGpio);
    GPIO_PinInit(i2cBus->sclGpio, i2cBus->sclPin, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});

    bool isOn = true;
    for (int i=0; i<20; i++) {
        GPIO_PinInit(i2cBus->sdaGpio, i2cBus->sdaPin, &(gpio_pin_config_t){kGPIO_DigitalInput});
        bool isSdaHigh = GPIO_PinRead(i2cBus->sdaGpio, i2cBus->sdaPin);
        GPIO_PinInit(i2cBus->sdaGpio, i2cBus->sdaPin, &(gpio_pin_config_t){kGPIO_DigitalOutput, 1});

        if (isSdaHigh) {
            return;
        }

        GPIO_PinWrite(i2cBus->sclGpio, i2cBus->sclPin, isOn);
        delay();
        isOn = !isOn;
    }

    GPIO_PinWrite(i2cBus->sdaGpio, i2cBus->sdaPin, 0);
    delay();
    GPIO_PinWrite(i2cBus->sclGpio, i2cBus->sclPin, 1);
    delay();
    GPIO_PinWrite(i2cBus->sdaGpio, i2cBus->sdaPin, 1);
    delay();
}

static void initI2cBus(i2c_bus_t *i2cBus)
{
    CLOCK_EnableClock(i2cBus->sdaClock);
    CLOCK_EnableClock(i2cBus->sclClock);

    recoverI2cBus(i2cBus);

    port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
        .openDrainEnable = kPORT_OpenDrainEnable,
        .mux = i2cBus->mux,
    };

    PORT_SetPinConfig(i2cBus->sdaPort, i2cBus->sdaPin, &pinConfig);
    PORT_SetPinConfig(i2cBus->sclPort, i2cBus->sclPin, &pinConfig);

    i2c_master_config_t masterConfig;
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = i2cBus == &i2cMainBus ? I2cMainBusRequestedBaudRateBps : I2C_EEPROM_BUS_BAUD_RATE;
    uint32_t sourceClock = CLOCK_GetFreq(i2cBus->clockSrc);
    uint32_t baudrate = I2C_MasterInit(i2cBus->baseAddr, &masterConfig, sourceClock);

    if (i2cBus == &i2cMainBus) {
        I2cMainBusActualBaudRateBps = baudrate;
    }
}

void ReinitI2cMainBus(void)
{
    if (SLAVE_PROTOCOL_OVER_UART) {
        return;
    }
    I2C_MasterDeinit(I2C_MAIN_BUS_BASEADDR);
    initI2cBus(&i2cMainBus);
    InitSlaveScheduler();
}

static void initI2c(void)
{
    initI2cBus(&i2cMainBus);
    initI2cBus(&i2cEepromBus);
}

void InitPeripherals(void)
{
    // PIT has multiple users, prepare first
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT, &pitConfig);

    initBusPalState();
    initInterruptPriorities();
    Timer_Init();
    InitLedDriver();
    InitResetButton();
    MergeSensor_Init();
    ADC_Init();
    initI2c();
    TestLed_Init();
    LedPwm_Init();
    InitI2cWatchdog();
    EEPROM_Init();
}

void ChangeI2cBaudRate(uint32_t i2cBaudRate)
{
    I2cMainBusRequestedBaudRateBps = i2cBaudRate;
    ReinitI2cMainBus();
}
