#ifndef __I2C_WATCHDOG_H__
#define __I2C_WATCHDOG_H__

// Includes:

    #include "peripherals/pit.h"

// Macros:

    #define I2C_WATCHDOG_INTERVAL_USEC 100000

// Variables:

    extern uint32_t I2cWatchdog_RecoveryCounter;
    extern uint32_t I2cWatchdog_WatchCounter;

// Functions:

    void InitI2cWatchdog(void);

#endif
