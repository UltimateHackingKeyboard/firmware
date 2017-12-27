#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_watchdog.h"
#include "test_led.h"
#include "init_peripherals.h"
#include "config.h"

/* NOTE: Because of a bug in the ROM bootloader of the KL03Z, the watchdog timer is disabled and cannot be re-enabled.
 * See https://community.nxp.com/thread/457893
 * Therefore the hardware watchdog timer cannot be used without an extra way to enter bootloader or application mode.
 */
#ifdef I2C_WATCHDOG
  static uint32_t prevWatchdogCounter = 0;
  static uint32_t I2cWatchdog_RecoveryCounter; /* counter for how many times we had to recover and restart */

void RunWatchdog(void)
{
    static volatile uint32_t I2cWatchdog_WatchCounter = 0; /* counter for timer */
    static int cntr = 0;

    cntr++;
    if (cntr==100) { /* we get called from KEY_SCANNER_HANDLER() which runs at 1ms, thus scaling down by 100 here to get 100 ms period */
        cntr=0;
        TEST_LED_TOGGLE();
        I2cWatchdog_WatchCounter++;

        if (I2cWatchdog_WatchCounter>10) { /* do not check within the first 1000 ms, as I2C might not be running yet */
            if (I2C_Watchdog == prevWatchdogCounter) { // Restart I2C if there hasn't been any interrupt during 100 ms. I2C_Watchdog gets incremented for every I2C transaction
                I2cWatchdog_RecoveryCounter++;
#if I2C_WATCHDOG == I2C_WATCHDOG_VALUE_REBOOT
                NVIC_SystemReset();
#endif
#if I2C_WATCHDOG == I2C_WATCHDOG_VALUE_REINIT
                I2C_SlaveDeinit(I2C_BUS_BASEADDR);
                InitI2c();
#endif
            }
        }
        prevWatchdogCounter = I2C_Watchdog; /* remember previous counter */
    }
}
#endif
