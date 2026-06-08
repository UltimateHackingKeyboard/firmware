#include "jitter_test.h"
#include "timer.h"
#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#endif

#define JITTER_TEST_SAMPLE_COUNT 50
#define JITTER_TEST_WINDOW_MS 1000

bool JitterTest_Active = false;

typedef struct {
    uint8_t dt;
    uint8_t x;
} jitter_sample_t;

static jitter_sample_t samples[JITTER_TEST_SAMPLE_COUNT];
static uint16_t count;
static uint32_t lastSampleTime;
static uint32_t windowStart;
static uint32_t nextWindowStart;
static bool waiting;

void JitterTest_SetActive(bool active)
{
    JitterTest_Active = active;
    count = 0;
    waiting = false;
    nextWindowStart = 0;
}

static void startWindow(uint32_t now)
{
    waiting = false;
    count = 0;
    windowStart = now;
    lastSampleTime = now;
}

static void dumpSamples(void)
{
#ifdef __ZEPHYR__
    printk("-----\n");
    for (uint32_t i = 0; i < count; i++) {
        printk("%u %d\n", samples[i].dt, samples[i].x);
    }
#endif
}

void JitterTest_RecordMouseX(int16_t x)
{
    if (!JitterTest_Active) {
        return;
    }

    uint32_t now = Timer_GetCurrentTime();

    if (waiting) {
        if (now < nextWindowStart) {
            return;
        }
        startWindow(now);
    } else if (count == 0) {
        startWindow(now);
    }

    samples[count].dt = now - lastSampleTime;
    samples[count].x = x;
    lastSampleTime = now;
    count++;

    if (count >= JITTER_TEST_SAMPLE_COUNT) {
        dumpSamples();
        waiting = true;
        nextWindowStart = windowStart + JITTER_TEST_WINDOW_MS;
    }
}
