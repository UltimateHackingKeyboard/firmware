#ifndef __TIMER_H__
#define __TIMER_H__

// Includes:

    #include "peripherals/pit.h"

// Macros:

    #define TIMER_INTERVAL_MSEC 1

// Variables:

    extern uint32_t CurrentTime;

// Functions:

    void Timer_Init(void);
    uint32_t Timer_GetElapsedTime(uint32_t *time);

#endif
