#include "fsl_pit.h"
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "i2c.h"
#include "i2c_watchdog.h"
#include "slave_scheduler.h"
#include "init_peripherals.h"

uint32_t I2cWatchdog_OuterCounter;
uint32_t I2cWatchdog_InnerCounter;

static uint32_t prevWatchdogCounter;

// This function restarts and reinstalls the I2C handler when the I2C bus gets unresponsive
// by a misbehaving I2C slave, or by disconnecting the left keyboard half or an add-on module.
// This method relies on a patched KSDK which increments I2C_Watchdog upon I2C transfers.
void PIT_I2C_WATCHDOG_HANDLER(void)
{
    I2cWatchdog_OuterCounter++;
    if (I2C_Watchdog == prevWatchdogCounter) { // Restart I2C if there haven't been any interrupts recently
        I2cWatchdog_InnerCounter++;
        i2c_master_config_t masterConfig;
        I2C_MasterGetDefaultConfig(&masterConfig);
        I2C_MasterDeinit(I2C_MAIN_BUS_BASEADDR);
        InitI2cMainBus();
        InitSlaveScheduler();
    }

    prevWatchdogCounter = I2C_Watchdog;

    PIT_ClearStatusFlags(PIT, kPIT_Chnl_0, PIT_TFLG_TIF_MASK);
}

void InitI2cWatchdog(void)
{
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT, &pitConfig);
    PIT_SetTimerPeriod(PIT, kPIT_Chnl_0, USEC_TO_COUNT(I2C_WATCHDOG_INTERVAL_USEC, PIT_SOURCE_CLOCK));
    PIT_EnableInterrupts(PIT, kPIT_Chnl_0, kPIT_TimerInterruptEnable);
    EnableIRQ(PIT_I2C_WATCHDOG_IRQ_ID);
    PIT_StartTimer(PIT, kPIT_Chnl_0);
}
