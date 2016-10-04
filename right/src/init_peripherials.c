#include "fsl_common.h"
#include "fsl_port.h"
#include "test_led.h"
#include "reset_button.h"
#include "i2c.h"
#include "led_driver.h"

void InitPeripherials(void)
{
    // Ungate ports.
    CLOCK_EnableClock(kCLOCK_PortA); // LEDs
    CLOCK_EnableClock(kCLOCK_PortB); // SW3
    CLOCK_EnableClock(kCLOCK_PortC); // SW2
    CLOCK_EnableClock(kCLOCK_PortD); // LEDs, I2C

    // Set up switches
    port_pin_config_t switchConfig = {
        .pullSelect = kPORT_PullUp,
        .mux = kPORT_MuxAsGpio,
    };
    PORT_SetPinConfig(RESET_BUTTON_PORT, RESET_BUTTON_PIN, &switchConfig);

    // Initialize LEDs.

    PORT_SetPinMux(TEST_LED_GPIO_PORT, TEST_LED_GPIO_PIN, kPORT_MuxAsGpio);
    TEST_LED_INIT(0);

    // Initialize I2C.

    port_pin_config_t pinConfig = {0};
    pinConfig.pullSelect = kPORT_PullUp;
    pinConfig.openDrainEnable = kPORT_OpenDrainEnable;

    PORT_SetPinConfig(PORTD, 2, &pinConfig);
    PORT_SetPinConfig(PORTD, 3, &pinConfig);

    PORT_SetPinMux(PORTD, 2, kPORT_MuxAlt7);
    PORT_SetPinMux(PORTD, 3, kPORT_MuxAlt7);

    i2c_master_config_t masterConfig;
    uint32_t sourceClock;
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2C_BAUD_RATE;
    sourceClock = CLOCK_GetFreq(I2C_MASTER_CLK_SRC);
    I2C_MasterInit(I2C_BASEADDR_MAIN_BUS, &masterConfig, sourceClock);
}
