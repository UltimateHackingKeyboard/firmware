#include "fsl_pit.h"
#include "timer.h"
#include "peripherals/test_led.h"

static uint32_t timerClockFrequency;
static volatile uint32_t currentTime, delayLength;

void PIT_TIMER_HANDLER(void)
{
    currentTime++;
    if (delayLength) {
        --delayLength;
    }
    PIT_ClearStatusFlags(PIT, PIT_TIMER_CHANNEL, kPIT_TimerFlag);
}

void Timer_Init(void)
{
    pit_config_t pitConfig;
    PIT_GetDefaultConfig(&pitConfig);
    PIT_Init(PIT, &pitConfig);

    timerClockFrequency = PIT_SOURCE_CLOCK;
    PIT_SetTimerPeriod(PIT, PIT_TIMER_CHANNEL, MSEC_TO_COUNT(TIMER_INTERVAL_MSEC, timerClockFrequency));

    PIT_EnableInterrupts(PIT, PIT_TIMER_CHANNEL, kPIT_TimerInterruptEnable);
    EnableIRQ(PIT_TIMER_IRQ_ID);
    PIT_StartTimer(PIT, PIT_TIMER_CHANNEL);
}

uint32_t Timer_GetCurrentTime() {
    return currentTime;
}

uint32_t Timer_GetCurrentTimeMicros() {
    uint32_t primask, count, ms;
    primask = DisableGlobalIRQ(); // Make sure the read is atomic
    count = PIT_GetCurrentTimerCount(PIT, PIT_TIMER_CHANNEL); // Read the current timer count
    ms = currentTime; // Read the overflow counter
    EnableGlobalIRQ(primask); // Enable interrupts again if they where enabled before - this should make it interrupt safe

    // Calculate the counter value in microseconds - note that the PIT timer is counting downward, so we need to subtract the count from the period value
    uint32_t us = 1000U * TIMER_INTERVAL_MSEC - COUNT_TO_USEC(count, timerClockFrequency);
    return ms * 1000U * TIMER_INTERVAL_MSEC + us;
}

void Timer_SetCurrentTime(uint32_t *time)
{
    *time = Timer_GetCurrentTime();
}

void Timer_SetCurrentTimeMicros(uint32_t *time)
{
    *time = Timer_GetCurrentTimeMicros();
}

uint32_t Timer_GetElapsedTime(uint32_t *time)
{
    uint32_t elapsedTime = Timer_GetCurrentTime() - *time;
    return elapsedTime;
}

uint32_t Timer_GetElapsedTimeMicros(uint32_t *time)
{
    uint32_t elapsedTime = Timer_GetCurrentTimeMicros() - *time;
    return elapsedTime;
}

uint32_t Timer_GetElapsedTimeAndSetCurrent(uint32_t *time)
{
    uint32_t elapsedTime = Timer_GetElapsedTime(time);
    *time = Timer_GetCurrentTime();
    return elapsedTime;
}

uint32_t Timer_GetElapsedTimeAndSetCurrentMicros(uint32_t *time)
{
    uint32_t elapsedTime = Timer_GetElapsedTimeMicros(time);
    *time = Timer_GetCurrentTimeMicros();
    return elapsedTime;
}

void Timer_Delay(uint32_t length)
{
    delayLength = length;
    while (delayLength) {
        ;
    }
}
