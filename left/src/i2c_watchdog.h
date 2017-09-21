#ifndef __I2C_WATCHDOG_H__
#define __I2C_WATCHDOG_H__

// Includes:

    #include "fsl_lptmr.h"

// Macros:

    #define I2C_WATCHDOG_LPTMR_HANDLER LPTMR0_IRQHandler
    #define LPTMR_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_LpoClk)
    #define LPTMR_USEC_COUNT 100000U

// Functions:

    extern void InitI2cWatchdog(void);
    extern void RunWatchdog(void);

#endif
