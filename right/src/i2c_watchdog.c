#include "fsl_pit.h"
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "i2c.h"
#include "i2c_watchdog.h"
#include "slave_scheduler.h"

#define PIT_I2C_WATCHDOG_HANDLER PIT0_IRQHandler
#define PIT_I2C_WATCHDOG_IRQ_ID PIT0_IRQn
#define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)

static uint32_t prevWatchdogCounter = 0;

// This function is designed to restart and reinstall the I2C handler
// when a disconnection of the left side makes the master I2C bus unresponsive.
void PIT_I2C_WATCHDOG_HANDLER(void)
{
    if (I2C_Watchdog == prevWatchdogCounter) { // Restart I2C if there hasn't be any interrupt during 1 sec
        i2c_master_config_t masterConfig;
        I2C_MasterGetDefaultConfig(&masterConfig);
        I2C_MasterDeinit(I2C_MAIN_BUS_BASEADDR);
        uint32_t sourceClock = CLOCK_GetFreq(I2C_MASTER_BUS_CLK_SRC);
        I2C_MasterInit(I2C_MAIN_BUS_BASEADDR, &masterConfig, sourceClock);
        InitBridgeProtocolScheduler();
    }

    prevWatchdogCounter = I2C_Watchdog;

    PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, PIT_TFLG_TIF_MASK);
}

void InitI2cWatchdog()
{
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT, &pitConfig);
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, USEC_TO_COUNT(1000000U, PIT_SOURCE_CLOCK));
    PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
    EnableIRQ(PIT_I2C_WATCHDOG_IRQ_ID);
    PIT_StartTimer(PIT, kPIT_Chnl_0);
}
