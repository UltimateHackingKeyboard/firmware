#ifndef __TIMER_H__
#define __TIMER_H__

// Includes:

    #include "peripherals/pit.h"

// Macros:

    #define TIMER_INTERVAL_USEC 1000

// Functions:

    void Timer_Init(void);
    uint32_t Timer_GetTime(void);

#endif
