#include "fsl_common.h"
#include "fsl_port.h"
#include "test_led.h"
#include "reset_button.h"
#include "i2c.h"
#include "led_driver.h"

void InitI2c() {
    port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
        .openDrainEnable = kPORT_OpenDrainEnable,
        .mux = kPORT_MuxAlt7
    };

    CLOCK_EnableClock(I2C_MAIN_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_MAIN_BUS_SCL_CLOCK);

    PORT_SetPinConfig(I2C_MAIN_BUS_SDA_PORT, I2C_MAIN_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_MAIN_BUS_SCL_PORT, I2C_MAIN_BUS_SCL_PIN, &pinConfig);

    i2c_master_config_t masterConfig;
    I2C_MasterGetDefaultConfig(&masterConfig);

    masterConfig.baudRate_Bps = I2C_MAIN_BUS_BAUD_RATE;
    uint32_t sourceClock = CLOCK_GetFreq(I2C_MASTER_BUS_CLK_SRC);
    I2C_MasterInit(I2C_MAIN_BUS_BASEADDR, &masterConfig, sourceClock);
}

void InitPeripherials(void)
{
    CLOCK_EnableClock(kCLOCK_PortA);
    CLOCK_EnableClock(kCLOCK_PortB);
    CLOCK_EnableClock(kCLOCK_PortC);
    CLOCK_EnableClock(kCLOCK_PortD);

    InitResetButton();
    InitTestLed();
    InitI2c();
}
