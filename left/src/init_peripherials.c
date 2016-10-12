#include "fsl_common.h"
#include "fsl_port.h"
#include "test_led.h"
#include "i2c_addresses.h"
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "i2c.h"

void InitI2c() {
    port_pin_config_t pinConfig = {
        .pullSelect = kPORT_PullUp,
    };

    i2c_master_config_t masterConfig;
    I2C_MasterGetDefaultConfig(&masterConfig);
    uint32_t sourceClock;

    // Initialize main bus

    CLOCK_EnableClock(I2C_BUS_SDA_CLOCK);
    CLOCK_EnableClock(I2C_BUS_SCL_CLOCK);

    pinConfig.mux = I2C_BUS_MUX;
    PORT_SetPinConfig(I2C_BUS_SDA_PORT, I2C_BUS_SDA_PIN, &pinConfig);
    PORT_SetPinConfig(I2C_BUS_SCL_PORT, I2C_BUS_SCL_PIN, &pinConfig);

    masterConfig.baudRate_Bps = I2C_BUS_BAUD_RATE;
    sourceClock = CLOCK_GetFreq(I2C_BUS_CLK_SRC);
    I2C_MasterInit(I2C_BUS_BASEADDR, &masterConfig, sourceClock);
}

void InitPeripherials(void)
{
    InitTestLed();
    InitI2c();
}
