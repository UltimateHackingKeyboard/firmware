#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_watchdog.h"
#include "test_led.h"
#include "init_peripherals.h"

/* NOTE: Because of a bug in the ROM bootloader of the KL03Z, the watchdog timer is disabled and cannot be re-enabled.
 * See https://community.nxp.com/thread/457893
 * Therefore the hardware watchdog timer cannot be used without an extra way to enter bootloader or application mode.
 */

#define WATCH_ENABLE_WATCHDOG   (1)  /* additionally, un-comment InitI2cWatchdog() in init_peripherals.c */

#if WATCH_ENABLE_WATCHDOG
static uint32_t prevWatchdogCounter = 0;
#endif
static uint32_t I2cWatchdog_RecoveryCounter; /* counter for how many times we had to recover and restart */

void InitI2cWatchdog(void)
{
    lptmr_config_t lptmrConfig;
    LPTMR_GetDefaultConfig(&lptmrConfig);
    LPTMR_Init(LPTMR0, &lptmrConfig);
    LPTMR_SetTimerPeriod(LPTMR0, USEC_TO_COUNT(LPTMR_USEC_COUNT, LPTMR_SOURCE_CLOCK)); /* set LPTM for a 100 ms period */
    LPTMR_EnableInterrupts(LPTMR0, kLPTMR_TimerInterruptEnable);
    EnableIRQ(LPTMR0_IRQn);
    LPTMR_StartTimer(LPTMR0);
}

#if WATCH_ENABLE_WATCHDOG
void I2C_WATCHDOG_LPTMR_HANDLER(void)
{
	static volatile uint32_t I2cWatchdog_WatchCounter = 0; /* counter for timer */
    TEST_LED_TOGGLE();
    I2cWatchdog_WatchCounter++;

    if (I2cWatchdog_WatchCounter>1) { /* do not check within the first 100 ms, as I2C might not be running yet */
		if (I2C_Watchdog == prevWatchdogCounter) { // Restart I2C if there hasn't been any interrupt during 100 ms. I2C_Watchdog gets incremented for every I2C transaction
	//        NVIC_SystemReset();
			I2cWatchdog_RecoveryCounter++;
			I2C_SlaveDeinit(I2C_BUS_BASEADDR);
			InitI2c();
		}
    }
    prevWatchdogCounter = I2C_Watchdog; /* remember previous counter */

    LPTMR_ClearStatusFlags(LPTMR0, kLPTMR_TimerCompareFlag);
}
#endif
