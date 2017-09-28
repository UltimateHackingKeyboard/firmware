#ifndef __I2C_WATCHDOG_H__
#define __I2C_WATCHDOG_H__

// Variables:

    extern uint32_t I2cWatchdog_InnerCounter;
    extern uint32_t I2cWatchdog_OuterCounter;

// Functions:

    void InitI2cWatchdog(void);

#endif
