#include "jitter_test.h"
#include "timer.h"
#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#endif

#define JITTER_TEST_SAMPLE_COUNT 50
#define JITTER_TEST_WINDOW_MS 1000

// Sample types recorded into the unified timeline.
#define JITTER_TYPE_REPORT 0       // a mouse report was built/sent
#define JITTER_TYPE_ANCHOR 1       // an anchor prepare fired (window was free)
#define JITTER_TYPE_ANCHOR_MISS 2  // anchor fired while previous window unconsumed

bool JitterTest_Active = false;

typedef struct {
    uint8_t dt;
    int8_t x;
    uint8_t type;
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
        printk("%u %d %u\n", samples[i].dt, samples[i].x, samples[i].type);
    }
#endif
}

// Append one event to the timeline. Recorded from multiple contexts (main loop
// for reports, system workqueue for anchors), so the append is done under an irq
// lock. The dump runs outside the lock; once a window is full we set
// waiting=true so no further samples are written until the next window, keeping
// the buffer stable while it is printed and keeping the printk out of the hot
// path being measured.
static void record(int16_t x, uint8_t type)
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
    samples[count].type = type;
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

void JitterTest_RecordMouseX(int16_t x)
{
    record(x, JITTER_TYPE_REPORT);
}

void JitterTest_RecordAnchor(bool windowWasOpen)
{
    record(0, windowWasOpen ? JITTER_TYPE_ANCHOR_MISS : JITTER_TYPE_ANCHOR);
}
