#ifndef __I2C_WATCHDOG_H__
#define __I2C_WATCHDOG_H__

// Variables:

    extern uint32_t I2C_WatchdogInnerCounter;
    extern uint32_t I2C_WatchdogOuterCounter;

// Functions:

    void InitI2cWatchdog(void);

#endif
