#include "fsl_i2c.h"
#include "include/board/clock_config.h"
#include "include/board/board.h"
#include "include/board/pin_mux.h"
#include "usb_composite_device.h"
#include "i2c.h"
#include "fsl_common.h"
#include "fsl_port.h"

void main() {
    BOARD_InitPins();
    BOARD_BootClockRUN();

    i2c_master_config_t masterConfig;
    uint32_t sourceClock;
    I2C_MasterGetDefaultConfig(&masterConfig);
    masterConfig.baudRate_Bps = I2C_BAUD_RATE;
    sourceClock = CLOCK_GetFreq(I2C_MASTER_CLK_SRC);
    I2C_MasterInit(I2C_BASEADDR_MAIN_BUS, &masterConfig, sourceClock);

    LedDriver_EnableAllLeds();
    InitUsb();

    while (1);
}
