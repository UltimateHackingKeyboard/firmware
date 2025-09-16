#ifndef __TIMER_H__
#define __TIMER_H__

// Includes:
    #include <stdint.h>

// Macros:

    #define TIMER_INTERVAL_MSEC 1

// Functions:
#ifndef __ZEPHYR__
    void Timer_Init(void);
    uint32_t Timer_GetCurrentTimeMicros(void);
    void Timer_SetCurrentTimeMicros(uint32_t *time);
#endif
    // returns the current system uptime in milliseconds
    uint32_t Timer_GetCurrentTime(void);
    uint32_t Timer_GetElapsedTime(uint32_t *time);
    uint32_t Timer_GetElapsedTimeMicros(uint32_t *time);
    uint32_t Timer_GetElapsedTimeAndSetCurrent(uint32_t *time);
#ifndef __ZEPHYR__
    uint32_t Timer_GetElapsedTimeAndSetCurrentMicros(uint32_t *time);
    void Timer_Delay(uint32_t length);
#endif

#endif
