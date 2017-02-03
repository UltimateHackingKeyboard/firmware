#include "fsl_common.h"
#include "fsl_port.h"
#include "test_led.h"
#include "reset_button.h"
#include "i2c.h"
#include "led_driver.h"
#include "merge_sensor.h"
#include "led_pwm.h"

void InitI2c() {
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

void InitPeripherials(void)
{
    InitResetButton();
    InitMergeSensor();
    InitI2c();
#if UHK_PCB_MAJOR_VERSION >= 7
    LedPwm_Init();
#endif
    InitTestLed(); // This function must not be called before LedPwm_Init() or else the UHK won't
                   // reenumerate over USB unless disconnecting it, waiting for at least 4 seconds
                   // and reconnecting it. This is the strangest thing ever!
}
