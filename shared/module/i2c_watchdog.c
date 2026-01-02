#include "fsl_i2c.h"
#include "i2c.h"
#include "i2c_watchdog.h"
#include "test_led.h"
#include "init_peripherals.h"
#include "module.h"

// NOTE: Because of a bug in the ROM bootloader of the KL03Z, the watchdog timer is disabled and cannot be re-enabled.
// See https://community.nxp.com/thread/457893
// Therefore the hardware watchdog timer cannot be used without an extra way to enter bootloader or application mode.
static uint32_t prevWatchdogCounter = 0;
static uint32_t I2cWatchdog_RecoveryCounter; // Counter for how many times we had to recover and restart

volatile uint32_t I2C_Watchdog = 0;
extern void I2C0_DriverIRQHandler(void);
void I2C0_IRQHandler(void)
{
    I2C_Watchdog++;
    I2C0_DriverIRQHandler();
}

void RunWatchdog(void)
{
    if (MODULE_OVER_UART) {
        return;
    }

    static volatile uint32_t I2cWatchdog_WatchCounter = 0; // Counter for timer
    static int counter = 0;

    counter++;
    if (counter == 100) { // We get called from KEY_SCANNER_HANDLER() which runs at 1ms, thus scaling down by 100 here to get 100 ms period
        counter=0;
        TestLed_Toggle();
        I2cWatchdog_WatchCounter++;

        if (I2cWatchdog_WatchCounter > 10) { // Do not check within the first 1000 ms, as I2C might not be running yet
            if (I2C_Watchdog == prevWatchdogCounter) { // Restart I2C if there hasn't been any interrupt during 100 ms. I2C_Watchdog gets incremented for every I2C transaction
                I2cWatchdog_RecoveryCounter++;
                I2C_SlaveDeinit(I2C_BUS_BASEADDR);
                initI2c();
            }
        }
        prevWatchdogCounter = I2C_Watchdog;
    }
}
