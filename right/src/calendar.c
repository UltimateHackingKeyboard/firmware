#include "calendar.h"
#include "segment_display.h"
#include "timer.h"
#include "macros.h"

uint32_t times[CalendarEvent_Count] = {};
calendar_event_t nextEvent;
uint32_t nextEventAt;
bool Calendar_IsActive = false;

static void scheduleNext()
{
    bool gotAny = false;
    for (uint8_t i = 0; i < CalendarEvent_Count; i++) {
        if (times[i] != 0 && (!gotAny || times[i] < nextEventAt)) {
            nextEvent = i;
            nextEventAt = times[i];
        }
    }
    if (!gotAny) {
        Calendar_IsActive = false;
    }
}

static void processEvt(calendar_event_t evt)
{
    switch (evt) {
        case CalendarEvent_SegmentDisplayUpdate:
            SegmentDisplay_NeedsUpdate = true;
            break;
        case CalendarEvent_MacroWakeOnTime:
            Macros_WakedBecauseOfTime = true;
            MacroPlaying = true;
            break;
        default:
            return;
    }
}


void Calendar_Reschedule(uint32_t at, calendar_event_t evt)
{
    times[evt] = at;
    if (nextEvent == evt) {
        scheduleNext();
    } else if (at < nextEventAt) {
        nextEventAt = at;
        nextEvent = evt;
    }
    Calendar_IsActive = true;
}

void Calendar_Schedule(uint32_t at, calendar_event_t evt)
{
    if (times[evt] == 0 || at < times[evt]) {
        times[evt] = at;
    }
    if (at < nextEventAt) {
        nextEventAt = at;
        nextEvent = evt;
    }
    Calendar_IsActive = true;
}


void Calendar_Process()
{
    if (nextEvent <= CurrentTime) {
        calendar_event_t evt = nextEvent;
        times[nextEvent] = 0;
        scheduleNext();
        processEvt(evt);
    }
}
