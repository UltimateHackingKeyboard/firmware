#include "microseconds/microseconds.h"
#include <stdarg.h>
#include "bus_pal_hardware.h"

enum {
    kFrequency_1MHz = 1000000UL
};

uint32_t s_tickPerMicrosecondMul8; // This value equals to 8 times ticks per microseconds

// Initialize the timer to lifetime timer by chained channel 0 and
// channel 1 together, and set both channels to maximum counting period.
void microseconds_init(void)
{
    uint32_t busClock;

    // PIT clock gate control ON
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;

    // Turn on PIT: MDIS = 0, FRZ = 0
    PIT->MCR = 0x00;

    // Set up timer 1 to max value
    PIT->CHANNEL[1].LDVAL = 0xFFFFFFFF;          // setup timer 1 for maximum counting period
    PIT->CHANNEL[1].TCTRL = 0;                   // Disable timer 1 interrupts
    PIT->CHANNEL[1].TFLG = 1;                    // clear the timer 1 flag
    PIT->CHANNEL[1].TCTRL |= PIT_TCTRL_CHN_MASK; // chain timer 1 to timer 0
    PIT->CHANNEL[1].TCTRL |= PIT_TCTRL_TEN_MASK; // start timer 1

    // Set up timer 0 to max value
    PIT->CHANNEL[0].LDVAL = 0xFFFFFFFF;         // setup timer 0 for maximum counting period
    PIT->CHANNEL[0].TFLG = 1;                   // clear the timer 0 flag
    PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TEN_MASK; // start timer 0

    // Calculate this value early
    // The reason why use this solution is that lowest clock frequency supported by L0PB and L4KS
    // is 0.25MHz, this solution will make sure ticks per microscond is greater than 0.

    busClock = get_bus_clock();
    s_tickPerMicrosecondMul8 = (busClock * 8) / kFrequency_1MHz;

    // Make sure this value is greater than 0
    if (!s_tickPerMicrosecondMul8) {
        s_tickPerMicrosecondMul8 = 1;
    }
}

void microseconds_shutdown(void)
{
    PIT->MCR |= PIT_MCR_MDIS_MASK; // Turn off PIT: MDIS = 1, FRZ = 0
}

uint64_t microseconds_get_ticks(void)
{
    uint64_t valueH;
    uint32_t valueL;

    // Make sure that there are no rollover of valueL.
    // Because the valueL always decreases, so, if the formal valueL is greater than
    // current value, that means the valueH is updated during read valueL.
    // In this case, we need to re-update valueH and valueL.
    do {
        valueH = PIT->CHANNEL[1].CVAL;
        valueL = PIT->CHANNEL[0].CVAL;
    } while (valueL < PIT->CHANNEL[0].CVAL);

    // Invert to turn into an up counter
    return ~((valueH << 32) | valueL);
}

// This is used to seperate any calculations from getting a timer
// value for speed critical scenarios
uint32_t microseconds_convert_to_microseconds(uint32_t ticks)
{
    // return the total ticks divided by the number of Mhz the system clock is at to give microseconds
    return (8 * ticks / s_tickPerMicrosecondMul8); // Assumes system clock will never be < 0.125 Mhz
}

uint64_t microseconds_convert_to_ticks(uint32_t microseconds)
{
    return ((uint64_t)microseconds * s_tickPerMicrosecondMul8 / 8);
}

void microseconds_delay(uint32_t us)
{
    uint64_t currentTicks = microseconds_get_ticks();
    // The clock value in Mhz = ticks/microsecond
    uint64_t ticksNeeded = ((uint64_t)us * s_tickPerMicrosecondMul8 / 8) + currentTicks;
    while (microseconds_get_ticks() < ticksNeeded) {
    }
}

// Get the clock value used for microseconds driver
uint32_t microseconds_get_clock(void)
{
    return get_bus_clock();
}
