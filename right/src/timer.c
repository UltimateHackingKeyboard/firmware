#include "fsl_pit.h"
#include "timer.h"

void Timer_Init(void)
{
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT, &pitConfig);
    PIT_SetTimerPeriod(PIT, PIT_TIMER_CHANNEL, USEC_TO_COUNT(TIMER_INTERVAL_USEC, PIT_SOURCE_CLOCK));
    PIT_EnableInterrupts(PIT, PIT_TIMER_CHANNEL, kPIT_TimerInterruptEnable);
//    EnableIRQ(PIT_TIMER_IRQ_ID);
    PIT_StartTimer(PIT, PIT_TIMER_CHANNEL);
}

uint32_t Timer_GetTime(void)
{
    return PIT_GetCurrentTimerCount(PIT, PIT_TIMER_CHANNEL);
}
