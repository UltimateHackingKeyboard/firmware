#include "relay_fake_under_test.inc"

// Model Schedule's existing contract: a later request cannot postpone a due
// timer. The actual scheduler event handler is extracted by the runner below.
enum { EventSchedulerEvent_UsbLeftRelay = 1 };
static uint32_t pollNow, pollDeadline;
static bool pollScheduled;
uint32_t Timer_GetCurrentTime(void) { return pollNow; }
static void EventScheduler_Schedule(uint32_t at, int evt, const char *label) {
    (void)label;
    assert(evt == EventSchedulerEvent_UsbLeftRelay);
    if (!pollScheduled || at < pollDeadline) pollDeadline = at;
    pollScheduled = true;
}
static void dispatchPollEvent(int evt) {
    switch (evt) {
#include "relay_poll_event_under_test.inc"
    default: assert(false);
    }
}

int main(void) {
    fake_t right, left;
    uint32_t now;
    initPair(&right, &left, &now);
    pollNow = now;
    EventScheduler_Schedule(now + 5, EventSchedulerEvent_UsbLeftRelay, "initial poll");
    // No keys, vendor commands or external wakeups: timer-driven polling must
    // sustain the session for several lease intervals by itself.
    for (unsigned i = 0; i < 5 * RELAY_LEASE_MS; ++i) {
        pollNow = ++now;
        if (pollScheduled && now >= pollDeadline) {
            Relay_Tick(&right.relay, now);
            EventScheduler_Schedule(now + 5, EventSchedulerEvent_UsbLeftRelay, "relay poll");
            // scheduleNextRun pops the already-due timer after UsbLeft_Process.
            pollScheduled = false;
            dispatchPollEvent(EventSchedulerEvent_UsbLeftRelay);
        }
        Relay_Tick(&left.relay, now);
        pump(&right);
        pump(&left);
        assert(right.relay.active && left.relay.active);
    }
    assert(pollScheduled && right.relay.heartbeat > 10);
    assert(!right.relay.faults && !left.relay.faults);
    puts("relay idle poll tests passed");
}
