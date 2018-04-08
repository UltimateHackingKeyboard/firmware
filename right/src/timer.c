#include "fsl_pit.h"
#include "timer.h"

static volatile uint32_t CurrentTime;

void PIT_TIMER_HANDLER(void)
{
    CurrentTime++;
    PIT_ClearStatusFlags(PIT, PIT_TIMER_CHANNEL, PIT_TFLG_TIF_MASK);
}

void Timer_Init(void)
{
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT, &pitConfig);

    // TODO: Why the interval needs to be multiplied by two to arrive to the correct timing?
    // Figure it out and clean this up.
    PIT_SetTimerPeriod(PIT, PIT_TIMER_CHANNEL, MSEC_TO_COUNT(TIMER_INTERVAL_MSEC*2, PIT_SOURCE_CLOCK));

    PIT_EnableInterrupts(PIT, PIT_TIMER_CHANNEL, kPIT_TimerInterruptEnable);
    EnableIRQ(PIT_TIMER_IRQ_ID);
    PIT_StartTimer(PIT, PIT_TIMER_CHANNEL);
}

uint32_t Timer_GetCurrentTime() {
    return CurrentTime;
}

void Timer_SetCurrentTime(uint32_t *time)
{
    *time = CurrentTime;
}

uint32_t Timer_GetElapsedTime(uint32_t *time)
{
    uint32_t elapsedTime = CurrentTime - *time;
    return elapsedTime;
}

uint32_t Timer_GetElapsedTimeAndSetCurrent(uint32_t *time)
{
    uint32_t elapsedTime = Timer_GetElapsedTime(time);
    *time = CurrentTime;
    return elapsedTime;
}
