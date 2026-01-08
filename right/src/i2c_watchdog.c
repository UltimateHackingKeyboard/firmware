#include "fsl_pit.h"
#include "fsl_i2c.h"
#include "fsl_clock.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "i2c.h"
#include "i2c_watchdog.h"
#include "init_peripherals.h"
#include "peripherals/test_led.h"
#include "trace.h"
#include "slave_protocol.h"

uint32_t I2cWatchdog_WatchCounter;
uint32_t I2cWatchdog_RecoveryCounter;

static uint32_t prevWatchdogCounter;

// This function restarts and reinstalls the I2C handler when the I2C bus gets unresponsive
// by a misbehaving I2C slave, or by disconnecting the left keyboard half or an add-on module.
// This method relies on a patched KSDK which increments I2C_Watchdog upon I2C transfers.
void PIT_I2C_WATCHDOG_HANDLER(void)
{
    if (SLAVE_PROTOCOL_OVER_UART) {
        return;
    }

    Trace_Printc("<i5");
    I2cWatchdog_WatchCounter++;

    if (I2C_Watchdog == prevWatchdogCounter) { // Restart I2C if there haven't been any interrupts recently
        I2cWatchdog_RecoveryCounter++;
        ReinitI2cMainBus();
    }

    prevWatchdogCounter = I2C_Watchdog;
    PIT_ClearStatusFlags(PIT, PIT_I2C_WATCHDOG_CHANNEL, PIT_TFLG_TIF_MASK);
	TestLed_Toggle();
    Trace_Printc(">");
    SDK_ISR_EXIT_BARRIER;
}

void InitI2cWatchdog(void)
{
    PIT_SetTimerPeriod(PIT, PIT_I2C_WATCHDOG_CHANNEL, USEC_TO_COUNT(I2C_WATCHDOG_INTERVAL_USEC, PIT_SOURCE_CLOCK));
    PIT_EnableInterrupts(PIT, PIT_I2C_WATCHDOG_CHANNEL, kPIT_TimerInterruptEnable);
    EnableIRQ(PIT_I2C_WATCHDOG_IRQ_ID);
    PIT_StartTimer(PIT, PIT_I2C_WATCHDOG_CHANNEL);
}
