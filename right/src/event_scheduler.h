#ifndef __EVENT_SCHEDULER_H__
#define __EVENT_SCHEDULER_H__

// Includes:

    #include <stdint.h>
    #include <stdbool.h>
    #include "timer.h"

// Macros:

// Typedefs:

    typedef enum {
        EventSchedulerEvent_SegmentDisplayUpdate,
        EventSchedulerEvent_MacroWakeOnTime,
        EventSchedulerEvent_Count
    } event_scheduler_event_t;

// Variables:

    extern bool EventScheduler_IsActive;

// Functions:

    void EventScheduler_Schedule(uint32_t at, event_scheduler_event_t evt);
    void EventScheduler_Reschedule(uint32_t at, event_scheduler_event_t evt);
    void EventScheduler_Process();

#endif
