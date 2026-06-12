#include "jitter_test.h"
#include "timer.h"
#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#endif

#define JITTER_TEST_SAMPLE_COUNT 50
#define JITTER_TEST_WINDOW_MS 1000

bool JitterTest_Active = false;

// One line per report. Columns: dt (ms since previous report), x (mouse delta),
// anchors (anchor prepares seen since the previous report; 1 == healthy 1:1),
// misses (of those anchors, how many fired while the previous window was still
// unconsumed == a late build).
typedef struct {
    uint8_t dt;
    int8_t x;
    uint8_t anchors;
    uint8_t misses;
} jitter_sample_t;

static jitter_sample_t samples[JITTER_TEST_SAMPLE_COUNT];
static uint16_t count;
static uint32_t lastSampleTime;
static uint32_t windowStart;
static uint32_t nextWindowStart;
static bool waiting;

// Anchor prepares accumulated since the last recorded report.
static uint8_t anchorsSinceReport;
static uint8_t missesSinceReport;

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
    anchorsSinceReport = 0;
    missesSinceReport = 0;
}

static void dumpSamples(void)
{
#ifdef __ZEPHYR__
    printk("-----\n");
    for (uint32_t i = 0; i < count; i++) {
        printk("%u %d %u %u\n",
            samples[i].dt, samples[i].x, samples[i].anchors, samples[i].misses);
    }
#endif
}

void JitterTest_RecordMouseX(int16_t x)
{
    if (!JitterTest_Active) {
        return;
    }

    bool doDump = false;
#ifdef __ZEPHYR__
    unsigned int key = irq_lock();
#endif

    uint32_t now = Timer_GetCurrentTime();

    if (waiting) {
        if (now < nextWindowStart) {
            goto out;
        }
        startWindow(now);
    } else if (count == 0) {
        startWindow(now);
    }

    samples[count].dt = now - lastSampleTime;
    samples[count].x = x;
    samples[count].anchors = anchorsSinceReport;
    samples[count].misses = missesSinceReport;
    anchorsSinceReport = 0;
    missesSinceReport = 0;
    lastSampleTime = now;
    count++;

    if (count >= JITTER_TEST_SAMPLE_COUNT) {
        waiting = true;
        nextWindowStart = windowStart + JITTER_TEST_WINDOW_MS;
        doDump = true;
    }

out:;
#ifdef __ZEPHYR__
    irq_unlock(key);
#endif

    if (doDump) {
        dumpSamples();
    }
}

// Recorded from the system workqueue (anchor prepare). Only bumps counters; the
// values surface on the next report line, so anchors never add their own lines.
void JitterTest_RecordAnchor(bool windowWasOpen)
{
    if (!JitterTest_Active) {
        return;
    }

#ifdef __ZEPHYR__
    unsigned int key = irq_lock();
#endif
    if (anchorsSinceReport < 255) {
        anchorsSinceReport++;
    }
    if (windowWasOpen && missesSinceReport < 255) {
        missesSinceReport++;
    }
#ifdef __ZEPHYR__
    irq_unlock(key);
#endif
}
