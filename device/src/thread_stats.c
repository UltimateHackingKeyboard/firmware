#include "thread_stats.h"
#include "main.h"
#include "zephyr/kernel/thread.h"
#include <zephyr/kernel.h>

#define THREAD_COUNT 30
#define MONITOR_INTERVAL_MS 10

typedef struct {
    uint64_t time;
    k_tid_t threadId;
    const char* name;
} thread_stats_t;

static bool enabled = true;

static uint64_t timeTotal;

static thread_stats_t threadStats[THREAD_COUNT];
static uint8_t threadCount = 0;

static uint64_t lastThreadStartTime = 0;
static k_tid_t lastThreadId = 0;

void ThreadStats_Init(void) {
    timeTotal = 0;
    threadCount = 0;
    enabled = true;
}

static uint8_t getThreadIdx(k_tid_t threadId) {
    const char* name = k_thread_name_get(threadId);
    for (uint8_t i = 0; i < threadCount; i++) {
        if (threadStats[i].name == name) {
            return i;
        }
    }

    if (threadCount < THREAD_COUNT) {
        uint8_t idx = threadCount++;
        threadStats[idx].threadId = threadId;;
        threadStats[idx].time = 0;
        threadStats[idx].name = k_thread_name_get(threadId);
        return idx;
    }

    return 255;
}

void ThreadStats_Switch(void) {
    if (!enabled) {
        return;
    }

    uint64_t now = k_cyc_to_us_near64(k_cycle_get_32());
    uint64_t diffUs = now - lastThreadStartTime;
    k_tid_t currentThreadId = k_sched_current_thread_query();

    // Add the time to the thread
    uint8_t threadIdx = getThreadIdx(lastThreadId);
    if (threadIdx != 255) {
        threadStats[threadIdx].time += diffUs;
        timeTotal += diffUs;
    }

    // Scale the values down
    if (timeTotal > (MONITOR_INTERVAL_MS/2)*1000) {
        timeTotal >>= 1;
        for (uint8_t i = 0; i < THREAD_COUNT; i++) {
            threadStats[i].time >>= 1;
        }
    }

    // sort the array
    if (threadIdx > 0 && threadStats[threadIdx].time > threadStats[threadIdx-1].time) {
        thread_stats_t tmp = threadStats[threadIdx];
        threadStats[threadIdx] = threadStats[threadIdx-1];
        threadStats[threadIdx-1] = tmp;
    }

    lastThreadId = currentThreadId;
    lastThreadStartTime = now;
}

void ThreadStats_Snap(void) {
    enabled = false;
}

void ThreadStats_Print(void) {
    ThreadStats_Snap();
    printk("Threads (%d), interval %d ms:\n", threadCount, MONITOR_INTERVAL_MS);
    for (uint8_t i = 0; i < threadCount; i++) {
        printk("    - %s: %d\n", threadStats[i].name, (uint32_t)(threadStats[i].time*100/timeTotal));
    }
    enabled = true;
}

#if DEBUG_THREAD_STATS
void sys_trace_thread_switched_in_user(void) {
    ThreadStats_Switch();
}
#endif

