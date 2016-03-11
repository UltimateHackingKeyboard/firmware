#include "fsl_i2c.h"
#include "include/board/clock_config.h"
#include "include/board/board.h"
#include "include/board/pin_mux.h"
#include "usb_composite_device.h"
#include "main.h"

void main(void)
{
    BOARD_InitPins();
    BOARD_BootClockHSRUN();
    BOARD_InitDebugConsole();

    i2c_master_config_t masterConfig;
    uint32_t sourceClock;
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2C_BAUD_RATE;
    sourceClock = CLOCK_GetFreq(I2C_MASTER_CLK_SRC);
    I2C_MasterInit(EXAMPLE_I2C_MASTER_BASEADDR, &masterConfig, sourceClock);

    InitUsb();

    while (1);
}
