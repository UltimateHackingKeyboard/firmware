#include "event_scheduler.h"
#ifdef __ZEPHYR__
#include "keyboard/oled/screens/screen_manager.h"
#include "keyboard/oled/oled.h"
#else
#include "segment_display.h"
#endif
#include "timer.h"
#include "macros/core.h"
#include "macro_recorder.h"
#include "utils.h"

uint32_t times[EventSchedulerEvent_Count] = {};
event_scheduler_event_t nextEvent;
uint32_t nextEventAt;
bool EventScheduler_IsActive = false;

static void scheduleNext()
{
    bool gotAny = false;
    for (uint8_t i = 0; i < EventSchedulerEvent_Count; i++) {
        if (times[i] != 0 && (!gotAny || times[i] < nextEventAt)) {
            gotAny = true;
            nextEvent = i;
            nextEventAt = times[i];
        }
    }
    EventScheduler_IsActive = gotAny;
}

static void processEvt(event_scheduler_event_t evt)
{
    switch (evt) {
        case EventSchedulerEvent_ShiftScreen:
#ifdef __ZEPHYR__
            Oled_ShiftScreen();
#endif
            break;
        case EventSchedulerEvent_SwitchScreen:
#ifdef __ZEPHYR__
            ScreenManager_SwitchScreenEvent();
#endif
            break;
        case EventSchedulerEvent_SegmentDisplayUpdate:
#ifndef __ZEPHYR__
            SegmentDisplay_NeedsUpdate = true;
#endif
            break;
        case EventSchedulerEvent_MacroWakeOnTime:
            Macros_WakedBecauseOfTime = true;
            MacroPlaying = true;
            break;
        case EventSchedulerEvent_MacroRecorderFlashing:
            MacroRecorder_UpdateRecordingLed();
            break;
        default:
            return;
    }
}


void EventScheduler_Reschedule(uint32_t at, event_scheduler_event_t evt)
{
    times[evt] = at;
    if (nextEvent == evt) {
        scheduleNext();
    }
    if (at < nextEventAt || !EventScheduler_IsActive) {
        nextEventAt = at;
        nextEvent = evt;
        EventScheduler_IsActive = true;
    }
}

void EventScheduler_Schedule(uint32_t at, event_scheduler_event_t evt)
{
    if (times[evt] == 0 || at < times[evt]) {
        times[evt] = at;
    }
    if (at < nextEventAt || !EventScheduler_IsActive) {
        nextEventAt = at;
        nextEvent = evt;
        EventScheduler_IsActive = true;
    }
}

void EventScheduler_Unschedule(event_scheduler_event_t evt)
{
    times[evt] = 0;

    if (nextEvent == evt) {
        scheduleNext();
    }
}

void EventScheduler_Process()
{
    if (nextEventAt <= CurrentTime) {
        event_scheduler_event_t evt = nextEvent;
        times[nextEvent] = 0;
        scheduleNext();
        processEvt(evt);
    }
}
