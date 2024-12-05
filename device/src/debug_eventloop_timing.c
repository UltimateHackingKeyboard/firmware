#include "debug_eventloop_timing.h"
#include "main.h"

static bool running = false;
static uint64_t startTime = 0;
static uint64_t outherThreadRunningTime = 0;
static uint64_t reportingTime = 0;
static uint64_t lastThreadStartTime = 0;
static uint64_t lastWatchTime = 0;
static uint64_t lastOtherThreadWatchTime = 0;
static k_tid_t lastThreadId;

void EventloopTiming_Start() {
    startTime = k_cyc_to_us_near64(k_cycle_get_32());
    lastThreadId = k_sched_current_thread_query();
    outherThreadRunningTime = 0;
    lastOtherThreadWatchTime = 0;
    lastWatchTime = startTime;
    reportingTime = 0;
    running = true;
}

void EventloopTiming_Switch() {
    if (lastThreadId != Main_ThreadId) {
        uint64_t thisTime = k_cyc_to_us_near64(k_cycle_get_32());
        outherThreadRunningTime += thisTime - lastThreadStartTime;
    }

    lastThreadId = k_sched_current_thread_query();
    lastThreadStartTime = k_cyc_to_us_near64(k_cycle_get_32());
}

void EventloopTiming_End() {
    uint64_t thisTime = k_cyc_to_us_near64(k_cycle_get_32());
    uint64_t runningTime = thisTime - startTime - outherThreadRunningTime - reportingTime;
    printk("User logic took %lld us, interrupted for %lld\n", runningTime, outherThreadRunningTime);
    running = false;
}

void EventloopTiming_WatchReset() {
    lastWatchTime = k_cyc_to_us_near64(k_cycle_get_32());
    lastOtherThreadWatchTime = outherThreadRunningTime;
}

void EventloopTiming_Watch(const char* section) {
    uint64_t now = k_cyc_to_us_near64(k_cycle_get_32());
    uint64_t currentInterruptions = outherThreadRunningTime - lastOtherThreadWatchTime;
    uint64_t currentRunningTime = now - lastWatchTime - currentInterruptions;

    printk("    %s took %lld\n", section, currentRunningTime);
    uint64_t now2 = k_cyc_to_us_near64(k_cycle_get_32());

    reportingTime += now2 - now;

    lastWatchTime = k_cyc_to_us_near64(k_cycle_get_32());
    lastOtherThreadWatchTime = outherThreadRunningTime;
}

void sys_trace_thread_switched_in_user(void) {
    if (running) {
        EventloopTiming_Switch();
    }
}

