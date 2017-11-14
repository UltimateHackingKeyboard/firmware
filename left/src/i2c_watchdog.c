#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_watchdog.h"
#include "test_led.h"
#include "init_peripherals.h"

//static uint32_t prevWatchdogCounter = 0;
uint32_t I2cWatchdog_RecoveryCounter;
volatile uint32_t I2cWatchdog_WatchCounter;

void InitI2cWatchdog(void)
{
    lptmr_config_t lptmrConfig;
    LPTMR_GetDefaultConfig(&lptmrConfig);
    LPTMR_Init(LPTMR0, &lptmrConfig);
    LPTMR_SetTimerPeriod(LPTMR0, USEC_TO_COUNT(LPTMR_USEC_COUNT, LPTMR_SOURCE_CLOCK));
    LPTMR_EnableInterrupts(LPTMR0, kLPTMR_TimerInterruptEnable);
    EnableIRQ(LPTMR0_IRQn);
    LPTMR_StartTimer(LPTMR0);
}
/*
void I2C_WATCHDOG_LPTMR_HANDLER(void)
{
    TEST_LED_TOGGLE();
    I2cWatchdog_WatchCounter++;

    if (I2C_Watchdog == prevWatchdogCounter) { // Restart I2C if there hasn't been any interrupt during 100 ms
//        NVIC_SystemReset();
        I2cWatchdog_RecoveryCounter++;
        I2C_SlaveDeinit(I2C_BUS_BASEADDR);
        InitI2c();
    }

    prevWatchdogCounter = I2C_Watchdog;

    LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);
}
*/
