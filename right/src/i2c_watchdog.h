#ifndef __I2C_WATCHDOG_H__
#define __I2C_WATCHDOG_H__

// Macros:

    #define I2C_WATCHDOG_INTERVAL_USEC 100000

    #define PIT_I2C_WATCHDOG_HANDLER PIT0_IRQHandler
    #define PIT_I2C_WATCHDOG_IRQ_ID PIT0_IRQn
    #define PIT_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_BusClk)

// Variables:

    extern uint32_t I2cWatchdog_InnerCounter;
    extern uint32_t I2cWatchdog_OuterCounter;

// Functions:

    void InitI2cWatchdog(void);

#endif
